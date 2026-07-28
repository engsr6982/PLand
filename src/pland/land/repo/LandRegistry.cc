#include "LandRegistry.h"
#include "StorageError.h"
#include "TransactionContext.h"
#include "internal/LandDimensionChunkMap.h"
#include "internal/LandIdAllocator.h"
#include "internal/LandMigrator.h"

#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/enums/LandRole.h"
#include "pland/land/Land.h"
#include "pland/land/LandTemplatePermTable.h"
#include "pland/land/observer/LandEventPublisher.h"
#include "pland/land/repo/LandContext.h"
#include "pland/land/validator/LandCreateValidator.h"
#include "pland/utils/JsonUtil.h"
#include "pland/utils/TimeUtils.h"


#include "ll/api/Expected.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/thread/ThreadPoolExecutor.h"

#include "mc/platform/UUID.h"
#include "mc/world/level/BlockPos.h"

#include "nlohmann/json_fwd.hpp"

#include "absl/container/flat_hash_map.h"

#include "fmt/core.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <ll/api/io/FileUtils.h>
#include <ll/api/thread/ServerThreadExecutor.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


namespace land {

namespace {
thread_local bool g_isInTransaction = false;
}

struct LandRegistry::Impl : public observer::LandEventPublisher {
    std::unique_ptr<ll::data::KeyValueDB>              mDB;                             // 领地数据库
    std::vector<mce::UUID>                             mLandOperators;                  // 领地操作员
    std::unordered_map<mce::UUID, PlayerSettings>      mPlayerSettings;                 // 玩家设置
    absl::flat_hash_map<LandID, std::shared_ptr<Land>> mLandCache;                      // 领地缓存
    mutable std::shared_mutex                          mMutex;                          // 读写锁
    std::unique_ptr<internal::LandIdAllocator>         mLandIdAllocator{nullptr};       // 领地ID分配器
    internal::LandDimensionChunkMap                    mDimensionChunkMap;              // 维度区块映射
    std::unique_ptr<LandTemplatePermTable>             mLandTemplatePermTable{nullptr}; // 领地模板权限表

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
                std::unique_lock lock(mMutex);
                updateIdx();
            }
        }
        LandEventPublisher::onOwnerChanged(land, oldOwner, newOwner);
    }
    void onMemberAdded(std::shared_ptr<Land> const& land, mce::UUID const& member) override {
        mMemberIdx.insert(member, land->getId());
        LandEventPublisher::onMemberAdded(land, member);
    }
    void onMemberRemoved(std::shared_ptr<Land> const& land, mce::UUID const& member) override {
        mMemberIdx.erase_value(member, land->getId());
        LandEventPublisher::onMemberRemoved(land, member);
    }
    void onMembersCleared(std::shared_ptr<Land> const& land) override {
        reverseClearMemberIdx(land);
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

    void loadOperators(ll::io::Logger& logger) {
        if (!mDB->has(DbOperatorDataKey)) {
            mDB->set(DbOperatorDataKey, "[]"); // empty array
        }
        auto ops = nlohmann::json::parse(*mDB->get(DbOperatorDataKey));
        for (auto& op : ops) {
            auto uuidStr = op.get<std::string>();
            if (!mce::UUID::canParse(uuidStr)) {
                logger.warn("Invalid operator UUID: {}", uuidStr);
                continue;
            }
            mLandOperators.emplace_back(uuidStr);
        }
    }

    void loadPlayerSettings() {
        if (!mDB->has(DbPlayerSettingDataKey)) {
            mDB->set(DbPlayerSettingDataKey, "{}"); // empty object
        }
        auto settings = nlohmann::json::parse(*mDB->get(DbPlayerSettingDataKey));
        if (!settings.is_object()) [[unlikely]] {
            throw std::runtime_error("player settings is not an object");
        }

        for (auto& [key, value] : settings.items()) {
            PlayerSettings obj;
            json_util::json2structWithDiffPatch(value, obj);
            mPlayerSettings.emplace(key, std::move(obj));
        }
    }

    void loadLands() {
        ll::coro::Generator<std::pair<std::string_view, std::string_view>> iter = mDB->iter();

        auto& landMigrator = internal::LandMigrator::getInstance();

        LandID safeId{0};
        for (auto [key, value] : iter) {
            if (!isLandData(key)) continue;

            auto json = nlohmann::json::parse(value);
            if (auto expected = landMigrator.migrate(json, LandSchemaVersion); !expected) {
                throw std::runtime_error{expected.error().message()};
            }

            auto land = Land::make();
            land->load(json);

            // 保证landID唯一
            if (safeId <= land->getId()) {
                safeId = land->getId() + 1;
            }

            initIndex(land);
            mLandCache.emplace(land->getId(), std::move(land));
        }

        mLandIdAllocator = std::make_unique<internal::LandIdAllocator>(safeId); // 初始化ID分配器
    }

    void loadTemplatePermTable(ll::io::Logger& logger) {
        if (!mDB->has(DbTemplatePermKey)) {
            auto t = LandPermTable{};
            mDB->set(DbTemplatePermKey, json_util::struct2json(t).dump());
        }

        auto rawJson = mDB->get(DbTemplatePermKey);
        try {
            auto json = nlohmann::json::parse(*rawJson);
            if (!json.is_object()) [[unlikely]] {
                throw std::runtime_error("Template perm table is not an object");
            }

            auto t = LandPermTable{};
            json_util::json2structWithDiffPatch(json, t); // 反射并补丁

            mLandTemplatePermTable = std::make_unique<LandTemplatePermTable>(t);
        } catch (...) {
            mLandTemplatePermTable = std::make_unique<LandTemplatePermTable>(LandPermTable{});
            logger.error("Failed to load template perm table, using default perm table instead");
        }
    }


    // ===========================
    // 数据库
    // ===========================

    /**
     * @brief 对数据库进行快照
     * @note 此任务会阻塞线程，请投递到线程池中执行
     */
    std::pair<std::string, int> snapshotDatabase(std::optional<std::string> const& dirName) const {
        auto& mod      = PLand::getInstance();
        auto  baseDir  = mod.getSelf().getDataDir() / SnapshotDir;
        auto  name     = dirName.value_or(std::to_string(time_utils::nowSeconds()));
        auto  finalDir = baseDir / name;

        int count = 0;
        {
            auto snapshot = std::make_unique<ll::data::KeyValueDB>(finalDir);
            ll::file_utils::writeFile(finalDir / ".incomplete", "");

            // iter() 底层会调用 leveldb::DB::NewIterator。
            // LevelDB 在底层生成了一个 MVCC（多版本并发控制）的只读快照，这里可以安全的跨线程读取。
            auto iter = mDB->iter();
            for (auto [key, value] : iter) {
                snapshot->set(key, value);
                count++;
            }

            std::filesystem::remove(finalDir / ".incomplete");
        }
        return {name, count};
    }

    enum class VersionState { VersionMatch, VersionTooHigh, VersionTooLow };
    std::pair<VersionState, int> ensureDatabaseVersion(ll::data::KeyValueDB& db, bool isNewDb) {
        if (!db.has(DbVersionKey)) {
            if (isNewDb) {
                db.set(DbVersionKey, std::to_string(LandSchemaVersion)); // 设置初始版本号
            } else {
                db.set(DbVersionKey, "-1"); // 数据库存在，但没有版本号，表示是旧版数据库(v0.8.1之前)
            }
        }

        auto version = std::stoi(*db.get(DbVersionKey));
        if (version != LandSchemaVersion) {
            auto state = version > LandSchemaVersion ? VersionState::VersionTooHigh : VersionState::VersionTooLow;
            return {state, version};
        }
        return {VersionState::VersionMatch, version};
    }

    void connectDatabase(PLand& mod) {
        auto&       self    = mod.getSelf();
        auto&       logger  = self.getLogger();
        auto const& dataDir = self.getDataDir();
        auto const  dbDir   = dataDir / DatabaseDir;

        bool const isNewCreatedDB = !std::filesystem::exists(dbDir); // 是否是新建的数据库

        auto backup = [&]() {
            auto const backupDir = dataDir / ("backup_db_" + std::to_string(time_utils::nowSeconds()));
            std::filesystem::copy(
                dbDir,
                backupDir,
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
            );
        };

        if (!mDB) {
            mDB = std::make_unique<ll::data::KeyValueDB>(dbDir);
        }

        auto state = ensureDatabaseVersion(*mDB, isNewCreatedDB);
        switch (state.first) {
        case VersionState::VersionMatch:
            return;
        case VersionState::VersionTooHigh: {
            logger.fatal(
                "The database version is too high, current version: {}, expected version: {}. In order to "
                "keep the data safe, the plugin refuses to load!",
                state.second,
                LandSchemaVersion
            );
            throw std::runtime_error("The database versions do not match");
        }
        case VersionState::VersionTooLow: {
            logger.warn(
                "The database version is too low, the current version: {}, the expected version: {}, the "
                "plugin will try to back up and upgrade the database...",
                state.second,
                LandSchemaVersion
            );
            mDB.reset();
            backup();
            mDB = std::make_unique<ll::data::KeyValueDB>(dbDir);
            mDB->set(DbVersionKey, std::to_string(LandSchemaVersion)); // 更新版本号
            // 这里只需要修改版本号以及备份，其它兼容转换操作将在 LandMigrator 中进行
            break;
        }
        }
    }

    void buildDimensionChunkMap() {
        for (auto& [id, land] : mLandCache) {
            mDimensionChunkMap.addLand(land);
        }
    }

    ll::Expected<> addLand(std::shared_ptr<Land> land, bool allocateId = true) {
        if (!land || (allocateId && land->getId() != INVALID_LAND_ID)) {
            return StorageError::make(StorageError::ErrorCode::InvalidLand, "The land is invalid or land ID is not -1");
        }
        if (allocateId) {
            land->_setLandId(mLandIdAllocator->nextId());
        }

        auto result = mLandCache.emplace(land->getId(), land);
        if (!result.second) {
            return StorageError::make(StorageError::ErrorCode::CacheMapError, "Failed to insert land into cache map");
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
            return StorageError::make(StorageError::ErrorCode::CacheMapError, "Failed to erase land from cache");
        }

        if (!this->mDB->del(std::to_string(ptr->getId()))) {
            mLandCache.emplace(ptr->getId(), ptr); // rollback
            mDimensionChunkMap.addLand(ptr);
            initIndex(ptr);
            return StorageError::make(StorageError::ErrorCode::DatabaseError, "Failed to delete land from database");
        }
        return {};
    }

    bool _save(std::shared_ptr<Land> const& land, bool force = false) const {
        if (!land->isDirty() && !force) {
            return true; // 没有变化，且非强制保存
        }
        if (mDB->set(std::to_string(land->getId()), land->toJson().dump())) {
            land->getDirtyCounter().reset();
            return true;
        }
        return false;
    }
};

LandID LandRegistry::_allocateNextId() { return impl->mLandIdAllocator->nextId(); }

LandRegistry::LandRegistry(PLand& mod) : impl(std::make_unique<Impl>()) {
    auto& logger = mod.getSelf().getLogger();

    logger.trace("打开数据库...");
    impl->connectDatabase(mod);

    auto lock = std::unique_lock<std::shared_mutex>(impl->mMutex);
    logger.info("加载管理员...");
    impl->loadOperators(logger);
    logger.info("已加载 {} 位管理员", impl->mLandOperators.size());

    logger.info("加载玩家个人设置...");
    impl->loadPlayerSettings();
    logger.info("已加载 {} 位玩家的个人设置", impl->mPlayerSettings.size());

    logger.info("加载领地数据...");
    impl->loadLands();
    logger.info("已加载 {} 个领地", impl->mLandCache.size());

    logger.info("加载领地默认权限模板...");
    impl->loadTemplatePermTable(logger);
    logger.info("领地默认权限模板加载完成");

    logger.info("构建领地空间索引...");
    impl->buildDimensionChunkMap();
    logger.info("领地空间索引构建完成");

    lock.unlock();
    logger.info("构建领地层级缓存...");
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
        save(); // 强制内存数据落盘

        auto& logger = PLand::getInstance().getSelf().getLogger();
        try {
            auto [name, count] = impl->snapshotDatabase(dirName);
            logger.info("Database snapshot [{}] created successfully ({} records).", name, count);
        } catch (std::exception const& exception) {
            logger.error("Failed to create database snapshot: {}", exception.what());
        }
        co_return;
    }).launch(mod.getThreadPool());
}


bool LandRegistry::isLandData(std::string_view key) {
    return key != DbVersionKey && key != DbOperatorDataKey && key != DbPlayerSettingDataKey && key != DbTemplatePermKey;
}

void LandRegistry::save() {
    std::shared_lock<std::shared_mutex> lock(impl->mMutex); // 获取锁
    impl->mDB->set(DbOperatorDataKey, json_util::struct2json(impl->mLandOperators).dump());

    impl->mDB->set(DbPlayerSettingDataKey, json_util::struct2json(impl->mPlayerSettings).dump());

    if (impl->mLandTemplatePermTable->isDirty()) {
        if (impl->mDB->set(DbTemplatePermKey, json_util::struct2json(impl->mLandTemplatePermTable->get()).dump())) {
            impl->mLandTemplatePermTable->resetDirty();
        }
    }

    for (auto const& land : impl->mLandCache | std::views::values) {
        (void)impl->_save(land, false);
    }
}

bool LandRegistry::save(std::shared_ptr<Land> const& land, bool force) const {
    std::unique_lock lock(impl->mMutex); // 获取锁
    return impl->_save(land, force);
}


bool LandRegistry::isOperator(mce::UUID const& uuid) const {
    std::shared_lock<std::shared_mutex> lock(impl->mMutex);
    return std::find(impl->mLandOperators.begin(), impl->mLandOperators.end(), uuid) != impl->mLandOperators.end();
}
bool LandRegistry::addOperator(mce::UUID const& uuid) {
    if (isOperator(uuid)) {
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(impl->mMutex); // 获取锁
    impl->mLandOperators.push_back(uuid);
    return true;
}
bool LandRegistry::removeOperator(mce::UUID const& uuid) {
    std::unique_lock<std::shared_mutex> lock(impl->mMutex); // 获取锁

    auto iter = std::find(impl->mLandOperators.begin(), impl->mLandOperators.end(), uuid);
    if (iter == impl->mLandOperators.end()) {
        return false;
    }
    impl->mLandOperators.erase(iter);
    return true;
}
std::vector<mce::UUID> const& LandRegistry::getOperators() const {
    std::shared_lock<std::shared_mutex> lock(impl->mMutex);
    return impl->mLandOperators;
}


PlayerSettings& LandRegistry::getOrCreatePlayerSettings(mce::UUID const& uuid) {
    std::shared_lock<std::shared_mutex> lock(impl->mMutex);

    auto iter = impl->mPlayerSettings.find(uuid);
    if (iter == impl->mPlayerSettings.end()) {
        iter = impl->mPlayerSettings.emplace(uuid, PlayerSettings{}).first;
    }
    return iter->second;
}

LandTemplatePermTable& LandRegistry::getLandTemplatePermTable() const { return *impl->mLandTemplatePermTable; }

bool LandRegistry::hasLand(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(impl->mMutex);
    return impl->mLandCache.find(id) != impl->mLandCache.end();
}

void LandRegistry::refreshLandRange(std::shared_ptr<Land> const& ptr) {
    std::unique_lock<std::shared_mutex> lock(impl->mMutex);
    impl->mDimensionChunkMap.refreshRange(ptr);
}

ll::Expected<> LandRegistry::addOrdinaryLand(std::shared_ptr<Land> const& land) {
    if (!land->isOrdinaryLand()) {
        return StorageError::make(
            StorageError::ErrorCode::LandTypeMismatch,
            "The land type does not match the required type"
        );
    }
    if (!LandCreateValidator::ensureLandRangeIsLegal(land->getAABB(), land->getDimensionId(), land->is3D())
        || !LandCreateValidator::ensureLandNotInForbiddenRange(land->getAABB(), land->getDimensionId())
        || !LandCreateValidator::ensureNoLandRangeConflict(*this, land)) {
        return StorageError::make(StorageError::ErrorCode::LandRangeIllegal, "The land range is illegal");
    }
    return impl->addLand(land);
}
ll::Expected<> LandRegistry::removeOrdinaryLand(std::shared_ptr<Land> const& ptr) {
    if (!ptr->isOrdinaryLand()) {
        return StorageError::make(
            StorageError::ErrorCode::LandTypeMismatch,
            "The land type does not match the required type"
        );
    }

    std::unique_lock lock(impl->mMutex); // 获取锁
    return impl->deleteLand(ptr);
}

ll::Expected<> LandRegistry::executeTransaction(
    std::unordered_set<std::shared_ptr<Land>> const& participants,
    TransactionCallback const&                       executor
) {
    std::unique_lock lock{impl->mMutex};

    struct TransactionGuard {
        TransactionGuard() { g_isInTransaction = true; }
        ~TransactionGuard() { g_isInTransaction = false; }
    } guard;

    struct Snapshot {
        LandContext context;
        int         dirtyCount;
    };
    std::unordered_map<Land*, Snapshot> snapshots;
    snapshots.reserve(participants.size());
    for (auto& land : participants) {
        snapshots[land.get()] = {land->_getContext(), land->getDirtyCounter().getCounter()};
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
        return StorageError::make(StorageError::ErrorCode::TransactionError, "Transaction aborted.");
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
    std::shared_lock lock(impl->mMutex);

    auto landIt = impl->mLandCache.find(id);
    if (landIt != impl->mLandCache.end()) {
        return landIt->second;
    }
    return nullptr;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands() const {
    std::shared_lock lock(impl->mMutex);

    std::vector<std::shared_ptr<Land>> lands;
    lands.reserve(impl->mLandCache.size());
    for (auto& land : impl->mLandCache) {
        lands.push_back(land.second);
    }
    return lands;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands(std::vector<LandID> const& ids) const {
    std::shared_lock lock(impl->mMutex);

    std::vector<std::shared_ptr<Land>> lands;
    for (auto id : ids) {
        if (auto iter = impl->mLandCache.find(id); iter != impl->mLandCache.end()) {
            lands.push_back(iter->second);
        }
    }
    return lands;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands(LandDimid dimid) const {
    std::shared_lock lock(impl->mMutex);

    std::vector<std::shared_ptr<Land>> lands;
    for (auto& land : impl->mLandCache) {
        if (land.second->getDimensionId() == dimid) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::vector<std::shared_ptr<Land>> LandRegistry::getLands(mce::UUID const& uuid, bool includeShared) const {
    std::shared_lock lock(impl->mMutex);

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
    std::shared_lock lock(impl->mMutex);

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
    std::shared_lock lock(impl->mMutex);

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
    std::shared_lock lock(impl->mMutex);

    if (includeOperator && isOperator(uuid)) return LandPermType::Admin;

    if (auto land = getLand(id); land) {
        return land->getPermType(uuid);
    }

    return LandPermType::Actor;
}


std::shared_ptr<Land> LandRegistry::getLandAt(BlockPos const& pos, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex>       lock(impl->mMutex);
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
    std::shared_lock<std::shared_mutex> lock(impl->mMutex);

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
    std::shared_lock<std::shared_mutex> lock(impl->mMutex);

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
    std::shared_lock<std::shared_mutex> lock(impl->mMutex);

    std::vector<std::shared_ptr<Land>> result;
    for (auto const& [id, land] : impl->mLandCache) {
        if (filter(land)) {
            result.push_back(land);
        }
    }
    return result;
}


} // namespace land
