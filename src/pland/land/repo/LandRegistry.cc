#include "LandRegistry.h"
#include "TransactionContext.h"
#include "internal/LandDatabase.h"
#include "internal/LandDimensionChunkMap.h"
#include "internal/LandFlushQueue.h"
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

    // 从 0 开始分配: INVALID_LAND_ID(-1) 是"未分配"哨兵, 不能作为首个领地 ID
    std::atomic<LandID> mNextLandID{0};

    internal::LandDimensionChunkMap        mDimensionChunkMap;              // 维度区块映射
    std::unique_ptr<LandTemplatePermTable> mLandTemplatePermTable{nullptr}; // 领地模板权限表

    std::unique_ptr<internal::LandFlushQueue> mFlushQueue; // 异步落盘队列

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
    void onMarkDirty(std::shared_ptr<Land> const& land) override {
        // 事务内抑制入队: 中间态可能被回滚, 提交阶段会统一入队最终快照
        if (g_isInTransaction) {
            return;
        }
        enqueueLandSnapshot(land);
    }

    /**
     * @brief 入队一块领地的序列化快照并唤醒 worker
     * @note 主线程是唯一的 Land 写者, 在 setter 调用链内序列化快照是安全的;
     *       worker 只消费载荷, 绝不触碰 Land 对象, 从而避免跨线程数据竞争
     */
    void enqueueLandSnapshot(std::shared_ptr<Land> const& land) {
        // 未分配 ID 的领地尚未注册进缓存, 数据会在 addLand/事务提交时重新入队,
        // 此处入队会写出 data:land_ctx:-1 的脏记录
        if (land->getId() == INVALID_LAND_ID) {
            PLand::getInstance().getSelf().getLogger().warn(
                "Skip enqueueing snapshot of land without an allocated ID "
                "(dirtyCount={}, owner={}, name={}, dim={}, parent={}, is3D={})",
                land->getDirtyCount(),
                land->getOwner().asString(),
                land->getName(),
                land->getDimensionId(),
                land->getParentLandID(),
                land->is3D()
            );
            return;
        }
        auto payload = internal::LandDatabase::serialize(land->_getContext());
        mFlushQueue->enqueueLand(land->getId(), land->getDirtyCount(), std::move(payload));
    }

    /// @brief 通知 worker 有 meta 键 (操作员等) 修改, 触发一次批量落盘
    void notifyMetaDirty() { mFlushQueue->notifyMetaDirty(); }

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

            // 防御: 无效 ID 记录直接跳过。加载阶段分配器尚未定锚 (mNextLandID 依赖已加载
            // 记录推进), 此时分配新 ID 可能与后续加载的原生 ID 冲突造成数据覆盖
            if (land->getId() == INVALID_LAND_ID) {
                logger.warn("Skipping land record without a valid ID ({} bytes)", view.as_str().size());
                continue;
            }

            // 保证landID唯一
            if (mNextLandID.load(std::memory_order_relaxed) <= land->getId()) {
                mNextLandID.store(land->getId() + 1, std::memory_order_relaxed);
            }

            initIndex(land); // 先挂 observer, 迁移产生的 markDirty 才能被入队持久化
            if (migrateResult.value() == infra::MigrateResult::Success) {
                land->markDirty();
            }
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

        // 异步删除: 入队删除任务, 由 flush worker 落盘, 不在主线程阻塞
        mFlushQueue->enqueueDelete(ptr->getId());
        ptr->setObserver(nullptr); // 解除观察: 防止删除后的 setter 把领地"幽灵写回"数据库
        return {};
    }

    /**
     * @brief 消费一批去重后的任务并落盘 (LandFlushQueue 注入的 consumer)
     * @note 只在线程池线程上运行; 消费的是入队时序列化的快照载荷,
     *       不读取任何 Land 对象字段, 与主线程的 Land 修改无数据竞争
     */
    bool consumeFlushBatch(std::vector<internal::FlushTask> const& tasks) {
        std::vector<std::pair<std::string, std::string>> puts;
        std::vector<std::string>                         dels;
        puts.reserve(tasks.size() + 3);
        dels.reserve(4);

        for (auto const& t : tasks) {
            if (t.mKind == internal::FlushTask::Kind::kLand) {
                // 任务所有权留在队列 (失败需重入队), 这里拷贝载荷
                puts.emplace_back(internal::LandDatabase::buildLandContextKey(t.mId), t.mPayload);
            } else {
                dels.push_back(internal::LandDatabase::buildLandContextKey(t.mId));
            }
        }

        // meta 键受互斥锁保护, worker 可安全序列化
        {
            std::shared_lock opLock(mOperatorMutex);
            puts.emplace_back(
                std::string{internal::LandDatabase::kAdminsKey},
                internal::LandDatabase::serialize(mAdmins)
            );
        }
        {
            std::shared_lock stLock(mSettingsMutex);
            puts.emplace_back(
                std::string{internal::LandDatabase::kPlayerSettingsKey},
                internal::LandDatabase::serialize(mPlayerSettings)
            );
        }
        if (mLandTemplatePermTable->isDirty()) {
            puts.emplace_back(
                std::string{internal::LandDatabase::kTemplatePermTableKey},
                internal::LandDatabase::serialize(mLandTemplatePermTable->get())
            );
        }

        if (!mDB->writeBatch(std::move(puts), std::move(dels))) {
            return false; // 由 LandFlushQueue 重新入队重试
        }

        // 精确回写脏计数: 仅清除"已落盘快照"覆盖的标记, 保留快照之后的新标记。
        // 事务提交依赖 isDirty() 决定是否入队, 若此处无条件清零会丢失提交
        std::shared_lock lock(mDataMutex);
        for (auto const& t : tasks) {
            if (t.mKind != internal::FlushTask::Kind::kLand) {
                continue;
            }
            if (auto it = mLandCache.find(t.mId); it != mLandCache.end()) {
                auto before   = it->second->getDirtyCount();
                auto snapshot = t.mDirtyCount;
                it->second->resetDirtyCounter(before >= snapshot ? before - snapshot : 0);
            }
        }
        mLandTemplatePermTable->resetDirty();
        return true;
    }
};

LandID LandRegistry::_allocateNextId() { return impl->allocateLandID(); }

LandRegistry::LandRegistry(PLand& mod) : impl(std::make_unique<Impl>()) {
    auto& logger = mod.getSelf().getLogger();

    // 必须在 loadLands 之前创建: 加载时挂上的 observer 会立即触发 enqueue
    impl->mFlushQueue =
        std::make_unique<internal::LandFlushQueue>([this](std::vector<internal::FlushTask> const& tasks) {
            return impl->consumeFlushBatch(tasks);
        });

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

    impl->mFlushQueue->start(mod.getThreadPool());
}

LandRegistry::~LandRegistry() {
    impl->mFlushQueue->stop(); // 等待 worker 完全退出 (含最终落盘), 避免 UAF
}

void LandRegistry::createSnapshot(std::optional<std::string> const& dirName) {
    auto& mod = PLand::getInstance();
    ll::coro::keepThis([this, dirName]() -> ll::coro::CoroTask<> {
        impl->mFlushQueue->flushAndWait(); // 等待异步队列完全落盘, 保证快照包含内存中的最新数据

        auto& mod    = PLand::getInstance();
        auto& logger = mod.getSelf().getLogger();

        std::filesystem::path finalPath;
        {
            // unique output dir
            auto const snapshotDir = mod.getSelf().getDataDir() / kSnapshotDir;

            auto secStr = std::to_string(time_utils::nowSeconds());

            auto outputDirName = dirName.value_or(secStr);
            {
                finalPath = snapshotDir / outputDirName;
                if (std::filesystem::exists(snapshotDir / outputDirName)) {
                    if (dirName) {
                        outputDirName += fmt::format("_{}", secStr);
                        finalPath      = snapshotDir / outputDirName;
                        logger.warn(
                            "Snapshot dir [{}] already exists, appending timestamp [{}] to avoid conflict",
                            *dirName,
                            secStr
                        );
                    }
                }
                if (std::filesystem::exists(finalPath)) {
                    logger.error("Snapshot dir [{}] already exists", finalPath);
                    co_return;
                }
            }
        }

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
    impl->notifyMetaDirty(); // 触发一次批量落盘, 操作员修改即时持久化
    return true;
}
bool LandRegistry::removeOperator(mce::UUID const& uuid) {
    std::unique_lock<std::shared_mutex> lock(impl->mOperatorMutex); // 获取锁

    auto iter = std::find(impl->mAdmins.begin(), impl->mAdmins.end(), uuid);
    if (iter == impl->mAdmins.end()) {
        return false;
    }
    impl->mAdmins.erase(iter);
    impl->notifyMetaDirty(); // 触发一次批量落盘, 操作员修改即时持久化
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
            // 注意：addLand 内部不要再分配 ID 了，因为已经分过了
            if (auto res = impl->addLand(land, false /* don't allocate id */); !res) {
                return res;
            }
        } else if (!land->isDirty()) {
            continue;
        }
        // 事务内的 markDirty 被抑制, 提交阶段统一入队最终快照
        impl->enqueueLandSnapshot(land);
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
    return getEffectiveRole(uuid, id, includeOperator);
}
LandRole LandRegistry::getEffectiveRole(mce::UUID const& uuid, LandID id, bool includeOperator) const {
    std::shared_lock lock(impl->mDataMutex);

    if (includeOperator) {
        std::shared_lock opLock(impl->mOperatorMutex);
        if (std::find(impl->mAdmins.begin(), impl->mAdmins.end(), uuid) != impl->mAdmins.end()) {
            return LandRole::Admin;
        }
    }

    if (id > INVALID_LAND_ID) {
        if (auto it = impl->mLandCache.find(id); it != impl->mLandCache.end()) {
            return it->second->getEffectiveRole(uuid);
        }
    }

    return LandRole::Actor;
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
