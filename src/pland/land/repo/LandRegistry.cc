#include "LandRegistry.h"
#include "TransactionContext.h"
#include "internal/LandDatabase.h"
#include "internal/LandDimensionChunkMap.h"
#include "internal/LandMigrator.h"
#include "internal/LegacyLandDatabaseReader.h"
#include "internal/LegacyLandDatabaseUpgrader.h"

#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/enums/LandRole.h"
#include "pland/land/Land.h"
#include "pland/land/LandTemplatePermTable.h"
#include "pland/land/observer/LandEventPublisher.h"
#include "pland/land/repo/LandContext.h"
#include "pland/land/validator/LandCreateValidator.h"
#include "pland/reflect/SerializeType.h"
#include "pland/utils/JsonUtil.h"
#include "pland/utils/TimeUtils.h"

#include "ll/api/Expected.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/thread/ThreadPoolExecutor.h"
#include <ll/api/io/FileUtils.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include "mc/platform/UUID.h"
#include "mc/world/level/BlockPos.h"

#include "absl/container/flat_hash_map.h"

#include "fmt/chrono.h"
#include "fmt/core.h"
#include "fmt/format.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace land {

namespace {
thread_local bool g_isInTransaction = false;
}

struct LandRegistry::Impl : public observer::LandEventPublisher {
    std::unique_ptr<internal::LandDatabase>            mDB;             // 领地数据库
    std::vector<mce::UUID>                             mAdmins;         // 领地操作员
    std::unordered_map<mce::UUID, PlayerSettings>      mPlayerSettings; // 玩家设置
    absl::flat_hash_map<LandID, std::shared_ptr<Land>> mLandCache;      // 领地缓存

    mutable std::shared_mutex mDataMutex;     // mLandCache, mDimensionChunkMap, mOwnerIdx, mMemberIdx
    mutable std::shared_mutex mOperatorMutex; // mLandOperators
    mutable std::shared_mutex mSettingsMutex; // mPlayerSettings

    std::atomic<LandID> mNextLandID{INVALID_LAND_ID};

    internal::LandDimensionChunkMap        mDimensionChunkMap;              // 维度区块映射
    std::unique_ptr<LandTemplatePermTable> mLandTemplatePermTable{nullptr}; // 领地模板权限表

    ll::coro::InterruptableSleep mInterruptableSleep; // 中断等待
    std::atomic_bool             mCoroAbort{false};   // 协程中断标志


    internal::BidirectionalMap<mce::UUID, LandID> mOwnerIdx;  // 领地主人索引
    internal::BidirectionalMap<mce::UUID, LandID> mMemberIdx; // 领地成员索引

    // ===========================
    // 索引更新
    // ===========================

    void
    onOwnerChanged(std::shared_ptr<Land> const& land, mce::UUID const& oldOwner, mce::UUID const& newOwner) override {
        if (oldOwner != newOwner) {
            // 更新索引
            auto updateIdx = [&]() {
                mOwnerIdx.erase_value(oldOwner, land->getId());
                mOwnerIdx.insert(newOwner, land->getId());
            };
            if (g_isInTransaction) {
                updateIdx();
            } else {
                std::unique_lock lock(mDataMutex);
                updateIdx();
            }
        }
        LandEventPublisher::onOwnerChanged(land, oldOwner, newOwner);
    }
    void onMemberAdded(std::shared_ptr<Land> const& land, mce::UUID const& member) override {
        if (g_isInTransaction) {
            mMemberIdx.insert(member, land->getId());
        } else {
            std::unique_lock lock(mDataMutex);
            mMemberIdx.insert(member, land->getId());
        }
        LandEventPublisher::onMemberAdded(land, member);
    }
    void onMemberRemoved(std::shared_ptr<Land> const& land, mce::UUID const& member) override {
        if (g_isInTransaction) {
            mMemberIdx.erase_value(member, land->getId());
        } else {
            std::unique_lock lock(mDataMutex);
            mMemberIdx.erase_value(member, land->getId());
        }
        LandEventPublisher::onMemberRemoved(land, member);
    }
    void onMembersCleared(std::shared_ptr<Land> const& land) override {
        if (g_isInTransaction) {
            reverseClearMemberIdx(land);
        } else {
            std::unique_lock lock(mDataMutex);
            reverseClearMemberIdx(land);
        }
        LandEventPublisher::onMembersCleared(land);
    }

    void reverseClearMemberIdx(std::shared_ptr<Land> const& land) {
        // 特殊情况: 成员被清空, 不知道 key, 使用双向表反查此领地关联的成员(key)
        auto id      = land->getId();
        auto members = mMemberIdx.reverse(id);
        for (auto& member : members) {
            mMemberIdx.erase_value(member, id);
        }
    }

    void initIndex(std::shared_ptr<Land> const& land) {
        land->setObserver(this);
        mOwnerIdx.insert(land->getOwner(), land->getId());
        for (auto& member : land->getMembers()) {
            mMemberIdx.insert(member, land->getId());
        }
    }

    void clearIndex(std::shared_ptr<Land> const& land) {
        mOwnerIdx.erase_value(land->getOwner(), land->getId());
        reverseClearMemberIdx(land);
    }


    // ===========================
    // 数据加载
    // ===========================

    ll::Expected<> loadAdmins() {
        if (mDB->has(internal::LandDatabase::kAdminsKey)) {
            if (auto ok = mDB->readTo(internal::LandDatabase::kAdminsKey, mAdmins); !ok) {
                return ll::makeStringError(fmt::format("Failed to load admins data: {}", ok.error().message()));
            }
        }
        return {};
    }
    ll::Expected<> loadPlayerSettings() {
        if (mDB->has(internal::LandDatabase::kPlayerSettingsKey)) {
            if (auto ok = mDB->readTo(internal::LandDatabase::kPlayerSettingsKey, mPlayerSettings); !ok) {
                return ll::makeStringError(fmt::format("Failed to load player settings: {}", ok.error().message()));
            }
        }
        return {};
    }
    void loadLands(ll::io::Logger& logger) {
        auto iter = mDB->iter(internal::LandDatabase::kLandContextPrefix);
        for (auto view : iter) {
            auto json = view.as_json();
            if (!json) {
                logger
                    .error("Failed to decode land record ({} bytes): {}", view.as_str().size(), json.error().message());
                continue;
            }

            auto migrateResult = internal::LandMigrator::getInstance().migrate(*json, kLandSchemaVersion);
            if (!migrateResult) {
                logger.error("Failed to migrate land record: {}", migrateResult.error().message());
                continue;
            }

            LandContext ctx;
            try {
                json_util::merge_versioned_and_deserialize<LandContext, nlohmann::json>(*json, ctx);
            } catch (std::exception const& e) {
                logger.error("Failed to deserialize land record: {}", e.what());
                continue;
            }

            auto land = Land::make(std::move(ctx));
            if (migrateResult.value() == infra::MigrateResult::Success) {
                land->markDirty();
            }

            // 保证landID唯一
            if (mNextLandID.load(std::memory_order_relaxed) <= land->getId()) {
                mNextLandID.store(land->getId() + 1, std::memory_order_relaxed);
            }

            initIndex(land);
            mLandCache.emplace(land->getId(), std::move(land));
        }
    }
    ll::Expected<> loadTemplatePermTable() {
        auto tab = LandPermTable{};

        if (!mDB->has(internal::LandDatabase::kTemplatePermTableKey)) {
            (void)mDB->save(internal::LandDatabase::kTemplatePermTableKey, tab); // 初始化默认数据
            mLandTemplatePermTable = std::make_unique<LandTemplatePermTable>(tab);
            return {};
        }

        auto expected          = mDB->readTo(internal::LandDatabase::kTemplatePermTableKey, tab);
        mLandTemplatePermTable = std::make_unique<LandTemplatePermTable>(tab); // load default

        if (!expected) {
            return ll::makeStringError(
                fmt::format("Failed to load template perm table: {}", expected.error().message())
            );
        }
        return {};
    }


    // ===========================
    // 数据库
    // ===========================
    void openDatabase(PLand& mod) {
        namespace fs = std::filesystem;

        auto& self    = mod.getSelf();
        auto& logger  = self.getLogger();
        auto& dataDir = self.getDataDir();

        auto const newDbDir    = internal::LandDatabase::getDatabasePath(dataDir);             // database_v2
        auto const legacyDbDir = internal::LegacyLandDatabaseReader::getDatabasePath(dataDir); // db (旧版)

        auto backup_db = [](fs::path const& sou, fs::path const& tarBaseDir, std::string_view dbName) {
            auto dirName   = fmt::format("backup_{}_{}", dbName, time_utils::nowSeconds());
            auto targetDir = tarBaseDir / dirName;
            std::filesystem::copy(
                sou,
                targetDir,
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
            );
        };

        bool const legacyDbExists = fs::exists(legacyDbDir);
        bool const newDbExists    = fs::exists(newDbDir);

        // 旧, 新
        // v, v => 中断加载
        // v, x => 升级数据
        // x, v => 正常加载
        // x, x => 新数据库

        if (newDbExists && legacyDbExists) {
            static auto last_write_time = [](fs::path const& path) {
                auto ftime = fs::last_write_time(path);
                using namespace std::chrono;
                auto sys_now  = system_clock::now();
                auto file_now = std::filesystem::file_time_type::clock::now();
                return sys_now + duration_cast<system_clock::duration>(ftime - file_now);
            };

            auto legacyTime = last_write_time(legacyDbDir);
            auto newTime    = last_write_time(newDbDir);
            logger.fatal(
                "CRITICAL: Both new database ({}) and legacy database ({}) exist simultaneously!\n"
                "  - Legacy DB last modified: {:%Y-%m-%d %H:%M:%S}\n"
                "  - New DB last modified: {:%Y-%m-%d %H:%M:%S}\n"
                "To prevent state desynchronization and data loss, startup has been aborted.\n"
                "Please manually inspect, backup, and remove one of the database directories.",
                legacyDbDir.string(),
                newDbDir.string(),
                legacyTime,
                newTime
            );
            throw std::runtime_error("Data consistency error: conflicting database environments detected.");
        }

        mDB = std::make_unique<internal::LandDatabase>(newDbDir);
        if (legacyDbExists && !newDbExists) {
            logger.info("Old version of the database detected, starting data migration...");
            auto legacyDb = std::make_unique<ll::data::KeyValueDB>(legacyDbDir);
            if (auto ok = internal::LegacyLandDatabaseUpgrader::upgrade(std::move(legacyDb), *mDB); !ok) {
                logger.fatal("Failed to migrate the database: {}", ok.error().message());
                throw std::runtime_error("Failed to migrate the database");
            }
            auto archiveName = fmt::format("legacy_{}", internal::LegacyLandDatabaseReader::kDatabaseDir);
            fs::rename(legacyDbDir, dataDir / archiveName);
        }
        if (!legacyDbExists && !newDbExists) {
            (void)mDB->setVersion(kLandSchemaVersion); // 新的空数据库
        }

        // 版本校验
        auto version = mDB->getVersion();
        if (version > kLandSchemaVersion) {
            logger.fatal(
                "The database version is too high, current version: {}, expected version: {}. In order to "
                "keep the data safe, the plugin refuses to load!",
                *version,
                kLandSchemaVersion
            );
            throw std::runtime_error("The database version is too high");
        }
        if (version < kLandSchemaVersion) {
            logger.warn(
                "The database version is too low, current version: {}, expected version: {}. Backing up and "
                "upgrading the database...",
                *version,
                kLandSchemaVersion
            );
            mDB.reset(); // 先释放文件锁再冷拷贝
            backup_db(newDbDir, dataDir, internal::LandDatabase::kDatabaseDirName);
            mDB = std::make_unique<internal::LandDatabase>(newDbDir);
            (void)mDB->setVersion(kLandSchemaVersion); // 数据迁移由加载时的 LandMigrator 逐条完成
        }
    }

    void buildDimensionChunkMap() {
        for (auto& [id, land] : mLandCache) {
            mDimensionChunkMap.addLand(land);
        }
    }

    [[nodiscard]] LandID allocateLandID() { return mNextLandID.fetch_add(1, std::memory_order_relaxed); }

    ll::Expected<> addLand(std::shared_ptr<Land> land, bool allocateId = true) {
        if (!land || (allocateId && land->getId() != INVALID_LAND_ID)) {
            return ll::makeStringError("Invalid land pointer or land ID is already allocated");
        }
        if (allocateId) {
            land->_setLandId(allocateLandID());
        }

        auto result = mLandCache.emplace(land->getId(), land);
        if (!result.second) {
            return ll::makeStringError(
                fmt::format("Failed to insert land {} into cache: duplicate ID detected", land->getId())
            );
        }

        mDimensionChunkMap.addLand(land);
        initIndex(land);
        land->markDirty(); // 标记为脏数据, 避免持久化失败
        return {};
    }
    ll::Expected<> deleteLand(std::shared_ptr<Land> const& ptr) {
        mDimensionChunkMap.removeLand(ptr);
        clearIndex(ptr);
        if (!mLandCache.erase(ptr->getId())) {
            mDimensionChunkMap.addLand(ptr);
            initIndex(ptr);
            return ll::makeStringError(fmt::format("Failed to erase land {} from cache", ptr->getId()));
        }

        if (!this->mDB->del(internal::LandDatabase::buildLandContextKey(ptr->getId()))) {
            mLandCache.emplace(ptr->getId(), ptr); // rollback
            mDimensionChunkMap.addLand(ptr);
            initIndex(ptr);
            return ll::makeStringError(fmt::format("Failed to delete land {} from database", ptr->getId()));
        }
        return {};
    }

    bool _save(std::shared_ptr<Land> const& land, bool force = false) const {
        if (!land->isDirty() && !force) {
            return true; // 没有变化，且非强制保存
        }
        if (mDB->save(internal::LandDatabase::buildLandContextKey(land->getId()), land->_getContext())) {
            land->resetDirtyCounter();
            return true;
        }
        return false;
    }
};

LandID LandRegistry::_allocateNextId() { return impl->allocateLandID(); }

LandRegistry::LandRegistry(PLand& mod) : impl(std::make_unique<Impl>()) {
    auto& logger = mod.getSelf().getLogger();

    impl->openDatabase(mod);

    auto lock = std::unique_lock(impl->mDataMutex);
    impl->loadAdmins();
    logger.info("已加载 {} 位管理员", impl->mAdmins.size());

    impl->loadPlayerSettings();
    logger.info("已加载 {} 位玩家的个人设置", impl->mPlayerSettings.size());

    impl->loadLands(logger);
    logger.info("已加载 {} 个领地", impl->mLandCache.size());

    impl->loadTemplatePermTable();
    logger.info("领地默认权限模板加载完成");

    impl->buildDimensionChunkMap();
    logger.info("领地空间索引构建完成");

    lock.unlock();
    {
        std::unordered_set<std::shared_ptr<Land>> familyTreeRoot{};
        for (auto& land : impl->mLandCache | std::views::values) {
            if (land->isParentLand()) {
                familyTreeRoot.insert(land);
            }
        }

        std::stack<std::pair<std::shared_ptr<Land>, int>> stack{};
        for (auto& root : familyTreeRoot) {
            stack.emplace(root, 0);

            while (!stack.empty()) {
                auto [curr, level] = stack.top();
                stack.pop();

                curr->_setCachedNestedLevel(level);
                if (curr->hasSubLand()) {
                    for (auto& child : getLands(curr->getSubLandIDs())) {
                        stack.emplace(child, level + 1);
                    }
                }
            }
        }
        logger.info("构建完成，共处理 {} 个领地组", familyTreeRoot.size());
    }

    ll::coro::keepThis([this]() -> ll::coro::CoroTask<> {
        while (!impl->mCoroAbort) {
            co_await impl->mInterruptableSleep.sleepFor(std::chrono::minutes{2});
            if (impl->mCoroAbort) {
                break;
            }
            save();
        }
        co_return;
    }).launch(mod.getThreadPool());
}

LandRegistry::~LandRegistry() {
    impl->mCoroAbort.store(true);
    impl->mInterruptableSleep.interrupt(true);
    try {
        save();
    } catch (std::exception const& exception) {
        PLand::getInstance().getSelf().getLogger().error(
            "Failed to save land registry during shutdown: {}",
            exception.what()
        );
    } catch (...) {
        PLand::getInstance().getSelf().getLogger().error("Failed to save land registry during shutdown: unknown error");
    }
}

void LandRegistry::createSnapshot(std::optional<std::string> const& dirName) {
    auto& mod = PLand::getInstance();
    ll::coro::keepThis([this, dirName]() -> ll::coro::CoroTask<> {
        save(); // 强制内存数据落盘 // TODO: 移除

        auto& mod           = PLand::getInstance();
        auto& logger        = mod.getSelf().getLogger();
        auto  snapshotDir   = mod.getSelf().getDataDir() / SnapshotDir;
        auto  outputDirName = dirName.value_or(std::to_string(time_utils::nowSeconds()));
        auto  finalPath     = snapshotDir / outputDirName;
        try {
            auto newDatabase = std::make_unique<ll::data::KeyValueDB>(finalPath);
            ll::file_utils::writeFile(finalPath / ".incomplete", "");

            auto count = impl->mDB->snapshotTo(*newDatabase);
            if (!count) {
                logger.error("Failed to snapshot database: {}", count.error().message());
            }

            std::filesystem::remove(finalPath / ".incomplete");
            logger.info("Database snapshot [{}] created successfully ({} records).", finalPath, *count);
        } catch (std::exception const& exception) {
            logger.error("Failed to create database snapshot: {}", exception.what());
        }
        co_return;
    }).launch(mod.getThreadPool());
}


void LandRegistry::save() {
    // 锁顺序: mDataMutex → mOperatorMutex → mSettingsMutex
    std::shared_lock dataLock(impl->mDataMutex);
    std::shared_lock opLock(impl->mOperatorMutex);
    std::shared_lock settingsLock(impl->mSettingsMutex);

    (void)impl->mDB->save(internal::LandDatabase::kAdminsKey, impl->mAdmins);
    (void)impl->mDB->save(internal::LandDatabase::kPlayerSettingsKey, impl->mPlayerSettings);

    // LandTemplatePermTable 自带内部锁，不需要额外保护
    if (impl->mLandTemplatePermTable->isDirty()) {
        if (impl->mDB->save(internal::LandDatabase::kTemplatePermTableKey, impl->mLandTemplatePermTable->get())) {
            impl->mLandTemplatePermTable->resetDirty();
        }
    }

    for (auto const& land : impl->mLandCache | std::views::values) {
        (void)impl->_save(land, false);
    }
}

bool LandRegistry::save(std::shared_ptr<Land> const& land, bool force) const {
    std::unique_lock lock(impl->mDataMutex);
    return impl->_save(land, force);
}


bool LandRegistry::isOperator(mce::UUID const& uuid) const {
    std::shared_lock<std::shared_mutex> lock(impl->mOperatorMutex);
    return std::find(impl->mAdmins.begin(), impl->mAdmins.end(), uuid) != impl->mAdmins.end();
}
bool LandRegistry::addOperator(mce::UUID const& uuid) {
    std::unique_lock<std::shared_mutex> lock(impl->mOperatorMutex);
    if (std::find(impl->mAdmins.begin(), impl->mAdmins.end(), uuid) != impl->mAdmins.end()) {
        return false;
    }
    impl->mAdmins.push_back(uuid);
    return true;
}
bool LandRegistry::removeOperator(mce::UUID const& uuid) {
    std::unique_lock<std::shared_mutex> lock(impl->mOperatorMutex); // 获取锁

    auto iter = std::find(impl->mAdmins.begin(), impl->mAdmins.end(), uuid);
    if (iter == impl->mAdmins.end()) {
        return false;
    }
    impl->mAdmins.erase(iter);
    return true;
}
std::vector<mce::UUID> LandRegistry::getOperators() const {
    std::shared_lock<std::shared_mutex> lock(impl->mOperatorMutex);
    return impl->mAdmins;
}


PlayerSettings& LandRegistry::getOrCreatePlayerSettings(mce::UUID const& uuid) {
    std::unique_lock<std::shared_mutex> lock(impl->mSettingsMutex);

    auto iter = impl->mPlayerSettings.find(uuid);
    if (iter == impl->mPlayerSettings.end()) {
        iter = impl->mPlayerSettings.emplace(uuid, PlayerSettings{}).first;
    }
    return iter->second;
}

LandTemplatePermTable& LandRegistry::getLandTemplatePermTable() const { return *impl->mLandTemplatePermTable; }

bool LandRegistry::hasLand(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(impl->mDataMutex);
    return impl->mLandCache.find(id) != impl->mLandCache.end();
}

void LandRegistry::refreshLandRange(std::shared_ptr<Land> const& ptr) {
    std::unique_lock<std::shared_mutex> lock(impl->mDataMutex);
    impl->mDimensionChunkMap.refreshRange(ptr);
}

ll::Expected<> LandRegistry::addOrdinaryLand(std::shared_ptr<Land> const& land) {
    if (!land->isOrdinaryLand()) {
        return ll::makeStringError("This land is not an ordinary land and cannot be added via addOrdinaryLand");
    }
    // 注意：validator 调用可能会访问 LandRegistry 的查询接口（如 ensureNoLandRangeConflict），
    // 这些查询接口各自加 shared_lock，与下面的 unique_lock 不会死锁，但存在 TOCTOU 窗口。
    if (!LandCreateValidator::ensureLandRangeIsLegal(land->getAABB(), land->getDimensionId(), land->is3D())
        || !LandCreateValidator::ensureLandNotInForbiddenRange(land->getAABB(), land->getDimensionId())
        || !LandCreateValidator::ensureNoLandRangeConflict(*this, land)) {
        return ll::makeStringError("The land AABB is illegal or conflicts with existing land");
    }
    std::unique_lock lock(impl->mDataMutex);
    return impl->addLand(land);
}
ll::Expected<> LandRegistry::removeOrdinaryLand(std::shared_ptr<Land> const& ptr) {
    if (!ptr->isOrdinaryLand()) {
        return ll::makeStringError("This land is not an ordinary land and cannot be removed via removeOrdinaryLand");
    }

    std::unique_lock lock(impl->mDataMutex); // 获取锁
    return impl->deleteLand(ptr);
}

ll::Expected<> LandRegistry::executeTransaction(
    std::unordered_set<std::shared_ptr<Land>> const& participants,
    TransactionCallback const&                       executor
) {
    std::unique_lock lock{impl->mDataMutex};

    struct TransactionGuard {
        TransactionGuard() { g_isInTransaction = true; }
        ~TransactionGuard() { g_isInTransaction = false; }
    } guard;

    struct Snapshot {
        LandContext context;
        uint32_t    dirtyCount;
    };
    std::unordered_map<Land*, Snapshot> snapshots;
    snapshots.reserve(participants.size());
    for (auto& land : participants) {
        snapshots[land.get()] = {land->_getContext(), land->getDirtyCount()};
    }

    TransactionContext ctx(*this);
    bool               success = false;
    try {
        success = executor(ctx);
    } catch (...) {
        success = false;
    }

    if (!success) {
        // === 回滚 (Rollback) ===
        for (auto& land : participants) {
            auto snapshot = snapshots[land.get()];
            land->_reinit(std::move(snapshot.context), snapshot.dirtyCount);
        }
        return ll::makeStringError("Transaction aborted: executor returned false or threw an exception");
    }

    // === 提交 (Commit) ===
    for (auto& land : participants) {
        if (ctx.mLandsToRemove.contains(land->getId())) {
            if (auto res = impl->deleteLand(land); !res) {
                PLand::getInstance().getSelf().getLogger().error("Failed to remove land during commit!");
            }
            continue;
        }

        // 处理新增 (有 ID 但不在 Cache 里)
        // 如果是新建的 SubLand，它现在有了 ID，但还没进 mLandCache。

        bool justAllocated =
            std::find(ctx.mAllocatedIds.begin(), ctx.mAllocatedIds.end(), land->getId()) != ctx.mAllocatedIds.end();

        if (justAllocated) {
            // 新领地，直接入库
            // 注意：_addLand 内部不要再分配 ID 了，因为已经分过了
            if (auto res = impl->addLand(land, false /* don't allocate id */); !res) {
                return res;
            }
        } else if (land->isDirty()) {
            (void)impl->_save(land, false);
        }
    }
    return {};
}

std::shared_ptr<Land> LandRegistry::getLand(LandID id) const {
    std::shared_lock lock(impl->mDataMutex);

    auto landIt = impl->mLandCache.find(id);
    if (landIt != impl->mLandCache.end()) {
        return landIt->second;
    }
    return nullptr;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands() const {
    std::shared_lock lock(impl->mDataMutex);

    std::vector<std::shared_ptr<Land>> lands;
    lands.reserve(impl->mLandCache.size());
    for (auto& land : impl->mLandCache) {
        lands.push_back(land.second);
    }
    return lands;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands(std::vector<LandID> const& ids) const {
    std::shared_lock lock(impl->mDataMutex);

    std::vector<std::shared_ptr<Land>> lands;
    for (auto id : ids) {
        if (auto iter = impl->mLandCache.find(id); iter != impl->mLandCache.end()) {
            lands.push_back(iter->second);
        }
    }
    return lands;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands(LandDimid dimid) const {
    std::shared_lock lock(impl->mDataMutex);

    std::vector<std::shared_ptr<Land>> lands;
    for (auto& land : impl->mLandCache) {
        if (land.second->getDimensionId() == dimid) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands(mce::UUID const& uuid, bool includeShared) const {
    std::shared_lock lock(impl->mDataMutex);

    std::vector<std::shared_ptr<Land>> lands;

    if (impl->mOwnerIdx.has_key(uuid)) {
        auto const& landIds = impl->mOwnerIdx.forward_at(uuid);
        lands.reserve(landIds.size());

        for (auto id : landIds) {
            if (auto it = impl->mLandCache.find(id); it != impl->mLandCache.end()) {
                lands.push_back(it->second);
            }
        }
    }

    if (includeShared) {
        if (impl->mMemberIdx.has_key(uuid)) {
            auto const& landIds = impl->mMemberIdx.forward_at(uuid);
            lands.reserve(lands.size() + landIds.size());
            for (auto id : landIds) {
                if (auto it = impl->mLandCache.find(id); it != impl->mLandCache.end()) {
                    lands.push_back(it->second);
                }
            }
        }
    }

    return lands;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands(mce::UUID const& uuid, LandDimid dimid) const {
    std::shared_lock lock(impl->mDataMutex);

    std::vector<std::shared_ptr<Land>> lands;

    if (impl->mOwnerIdx.has_key(uuid)) {
        auto const& landIds = impl->mOwnerIdx.forward_at(uuid);
        lands.reserve(landIds.size());

        for (auto id : landIds) {
            auto it = impl->mLandCache.find(id);
            if (it != impl->mLandCache.end() && it->second->getDimensionId() == dimid) {
                lands.push_back(it->second);
            }
        }
    }
    return lands;
}
std::unordered_map<mce::UUID, std::unordered_set<std::shared_ptr<Land>>> LandRegistry::getLandsByOwner() const {
    std::shared_lock lock(impl->mDataMutex);

    std::unordered_map<mce::UUID, std::unordered_set<std::shared_ptr<Land>>> result;
    result.reserve(impl->mOwnerIdx.forward_map().size());

    // 转换为 STL 容器
    for (auto const& [uuid, ids] : impl->mOwnerIdx.forward_map()) {
        auto& landSet = result[uuid];
        for (auto id : ids) {
            if (auto it = impl->mLandCache.find(id); it != impl->mLandCache.end()) {
                landSet.insert(it->second);
            }
        }
    }
    return result;
}


LandPermType LandRegistry::getPermType(mce::UUID const& uuid, LandID id, bool includeOperator) const {
    std::shared_lock lock(impl->mDataMutex);

    if (includeOperator) {
        std::shared_lock opLock(impl->mOperatorMutex);
        if (std::find(impl->mAdmins.begin(), impl->mAdmins.end(), uuid) != impl->mAdmins.end()) {
            return LandPermType::Admin;
        }
    }

    if (auto it = impl->mLandCache.find(id); it != impl->mLandCache.end()) {
        return it->second->getPermType(uuid);
    }

    return LandPermType::Actor;
}


std::shared_ptr<Land> LandRegistry::getLandAt(BlockPos const& pos, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex>       lock(impl->mDataMutex);
    std::unordered_set<std::shared_ptr<Land>> result;

    auto landsIds = impl->mDimensionChunkMap.queryLand(dimid, internal::ChunkEncoder::encode(pos.x >> 4, pos.z >> 4));
    if (!landsIds) {
        return nullptr;
    }

    for (auto const& id : *landsIds) {
        if (auto iter = impl->mLandCache.find(id); iter != impl->mLandCache.end()) {
            if (auto const& land = iter->second; land->getAABB().hasPos(pos, land->is3D())) {
                result.insert(land);
            }
        }
    }

    if (!result.empty()) {
        if (result.size() == 1) {
            return *result.begin(); // 只有一个领地，即普通领地
        }

        // 子领地优先级最高
        std::shared_ptr<Land> deepestLand = nullptr;
        int                   maxLevel    = -1;
        for (auto& land : result) {
            int currentLevel = land->getNestedLevel();
            if (currentLevel > maxLevel) {
                maxLevel    = currentLevel;
                deepestLand = land;
            }
        }
        return deepestLand;
    }

    return nullptr;
}
std::unordered_set<std::shared_ptr<Land>>
LandRegistry::getLandAt(BlockPos const& center, int radius, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(impl->mDataMutex);

    if (!impl->mDimensionChunkMap.hasDimension(dimid)) {
        return {};
    }

    std::unordered_set<internal::ChunkID>     visitedChunks; // 记录已访问的区块
    std::unordered_set<std::shared_ptr<Land>> lands;

    int minChunkX = (center.x - radius) >> 4;
    int minChunkZ = (center.z - radius) >> 4;
    int maxChunkX = (center.x + radius) >> 4;
    int maxChunkZ = (center.z + radius) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            internal::ChunkID chunkId = internal::ChunkEncoder::encode(x, z);
            if (visitedChunks.find(chunkId) != visitedChunks.end()) {
                continue; // 如果区块已经访问过，则跳过
            }
            visitedChunks.insert(chunkId);

            auto landsIds = impl->mDimensionChunkMap.queryLand(dimid, chunkId);
            if (!landsIds) {
                continue;
            }

            for (auto const& id : *landsIds) {
                if (auto iter = impl->mLandCache.find(id); iter != impl->mLandCache.end()) {
                    if (auto const& land = iter->second; land->isCollision(center, radius)) {
                        lands.insert(land);
                    }
                }
            }
        }
    }
    return lands;
}
std::unordered_set<std::shared_ptr<Land>>
LandRegistry::getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(impl->mDataMutex);

    if (!impl->mDimensionChunkMap.hasDimension(dimid)) {
        return {};
    }

    std::unordered_set<internal::ChunkID>     visitedChunks;
    std::unordered_set<std::shared_ptr<Land>> lands;

    int minChunkX = std::min(pos1.x, pos2.x) >> 4;
    int minChunkZ = std::min(pos1.z, pos2.z) >> 4;
    int maxChunkX = std::max(pos1.x, pos2.x) >> 4;
    int maxChunkZ = std::max(pos1.z, pos2.z) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            internal::ChunkID chunkId = internal::ChunkEncoder::encode(x, z);
            if (visitedChunks.find(chunkId) != visitedChunks.end()) {
                continue;
            }
            visitedChunks.insert(chunkId);

            auto landsIds = impl->mDimensionChunkMap.queryLand(dimid, chunkId);
            if (!landsIds) {
                continue;
            }

            for (auto const& id : *landsIds) {
                if (auto iter = impl->mLandCache.find(id); iter != impl->mLandCache.end()) {
                    if (auto const& land = iter->second; land->isCollision(pos1, pos2)) {
                        lands.insert(land);
                    }
                }
            }
        }
    }
    return lands;
}

std::vector<std::shared_ptr<Land>> LandRegistry::getLandsWhere(CustomFilter const& filter) const {
    std::shared_lock<std::shared_mutex> lock(impl->mDataMutex);

    std::vector<std::shared_ptr<Land>> result;
    for (auto const& [id, land] : impl->mLandCache) {
        if (filter(land)) {
            result.push_back(land);
        }
    }
    return result;
}


} // namespace land
