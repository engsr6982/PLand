#include "LandRegistry.h"
#include "LandCreateValidator.h"
#include "fmt/core.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/i18n/I18n.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "nlohmann/json_fwd.hpp"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/Land.h"
#include "pland/land/LandContext.h"
#include "pland/land/LandTemplatePermTable.h"
#include "pland/utils/JSON.h"
#include "pland/utils/Utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <expected>
#include <filesystem>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>


namespace land {
std::string PlayerSettings::SYSTEM_LOCALE_CODE() { return "system"; }
std::string PlayerSettings::SERVER_LOCALE_CODE() { return "server"; }


void LandRegistry::_loadOperators() {
    if (!mDB->has(DbOperatorDataKey)) {
        mDB->set(DbOperatorDataKey, "[]"); // empty array
    }
    auto ops = JSON::parse(*mDB->get(DbOperatorDataKey));
    JSON::jsonToStructNoMerge(ops, mLandOperators);
}

void LandRegistry::_loadPlayerSettings() {
    if (!mDB->has(DbPlayerSettingDataKey)) {
        mDB->set(DbPlayerSettingDataKey, "{}"); // empty object
    }
    auto settings = JSON::parse(*mDB->get(DbPlayerSettingDataKey));
    if (!settings.is_object()) {
        throw std::runtime_error("player settings is not an object");
    }

    for (auto& [key, value] : settings.items()) {
        PlayerSettings settings_;
        JSON::jsonToStructTryPatch(value, settings_);
        mPlayerSettings.emplace(key, std::move(settings_));
    }
}

void LandRegistry::_connectDatabaseAndCheckVersion() {
    auto&       self    = land::PLand::getInstance().getSelf();
    auto&       logger  = self.getLogger();
    auto const& dataDir = self.getDataDir();
    auto const  dbDir   = dataDir / DbDirName;

    bool const isNewCreatedDB = !fs::exists(dbDir); // 是否是新建的数据库

    auto backup = [&]() {
        auto const backupDir =
            dataDir
            / ("backup_db_" + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));
        fs::copy(dbDir, backupDir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    };

    if (!mDB) {
        mDB = std::make_unique<ll::data::KeyValueDB>(dbDir);
    }

    if (!mDB->has(DbVersionKey)) {
        if (isNewCreatedDB) {
            mDB->set(DbVersionKey, std::to_string(LandContextVersion)); // 设置初始版本号
        } else {
            mDB->set(DbVersionKey, "-1"); // 数据库存在，但没有版本号，表示是旧版数据库(0.8.1之前)
        }
    }

    auto version = std::stoi(*mDB->get(DbVersionKey));
    if (version != LandContextVersion) {
        if (version > LandContextVersion) {
            logger.fatal(
                "数据库版本过高，当前版本: {}, 期望版本: {}。为了保证数据安全，插件拒绝加载！",
                version,
                LandContextVersion
            );
            logger.fatal(
                "The database version is too high, current version: {}, expected version: {}. In order to "
                "keep the data safe, the plugin refuses to load!",
                version,
                LandContextVersion
            );
            throw std::runtime_error("The database versions do not match");

        } else if (version < LandContextVersion) {
            logger.warn(
                "数据库版本过低，当前版本: {}, 期望版本: {}，插件将尝试备份并升级数据库...",
                version,
                LandContextVersion
            );
            logger.warn(
                "The database version is too low, the current version: {}, the expected version: {}, the "
                "plugin will try to back up and upgrade the database...",
                version,
                LandContextVersion
            );
            mDB.reset();
            backup();
            mDB = std::make_unique<ll::data::KeyValueDB>(dbDir);
            mDB->set(DbVersionKey, std::to_string(LandContextVersion)); // 更新版本号
            // 这里只需要修改版本号以及备份，其它兼容转换操作将在 _checkVersionAndTryAdaptBreakingChanges 中进行
        }
    }
}

void LandRegistry::_checkVersionAndTryAdaptBreakingChanges(nlohmann::json& landData) {
    constexpr int LANDDATA_NEW_POS_KEY_VERSION = 15; // 在此版本后，LandAABB 使用了新的键名

    if (landData["version"].get<int>() < LANDDATA_NEW_POS_KEY_VERSION) {
        constexpr auto LEGACY_MAX_KEY = "mMax_B";
        constexpr auto LEGACY_MIN_KEY = "mMin_A";
        constexpr auto NEW_MAX_KEY    = "max";
        constexpr auto NEW_MIN_KEY    = "min";

        auto& pos = landData["mPos"];
        if (pos.contains(LEGACY_MAX_KEY)) {
            auto legacyMax = pos[LEGACY_MAX_KEY]; // copy
            pos.erase(LEGACY_MAX_KEY);
            pos[NEW_MAX_KEY] = std::move(legacyMax);
        }
        if (pos.contains(LEGACY_MIN_KEY)) {
            auto legacyMin = pos[LEGACY_MIN_KEY]; // copy
            pos.erase(LEGACY_MIN_KEY);
            pos[NEW_MIN_KEY] = std::move(legacyMin);
        }
    }
}

bool LandRegistry::isLandData(std::string_view key) {
    return key != DbVersionKey && key != DbOperatorDataKey && key != DbPlayerSettingDataKey && key != DbTemplatePermKey;
}
void LandRegistry::_loadLands() {
    ll::coro::Generator<std::pair<std::string_view, std::string_view>> iter = mDB->iter();

    LandID safeId{0};
    for (auto [key, value] : iter) {
        if (!isLandData(key)) continue;

        auto json = JSON::parse(value);
        _checkVersionAndTryAdaptBreakingChanges(json);

        auto land = Land::make();
        land->load(json);

        // 保证landID唯一
        if (safeId <= land->getId()) {
            safeId = land->getId() + 1;
        }

        mLandCache.emplace(land->getId(), std::move(land));
    }

    mLandIdAllocator = std::make_unique<LandIdAllocator>(safeId); // 初始化ID分配器
}
void LandRegistry::_loadLandTemplatePermTable() {
    if (!mDB->has(DbTemplatePermKey)) {
        auto t = LandPermTable{};
        mDB->set(DbTemplatePermKey, JSON::structTojson(t).dump());
    }

    auto rawJson = mDB->get(DbTemplatePermKey);
    try {
        auto json = nlohmann::json::parse(*rawJson);
        if (!json.is_object()) {
            throw std::runtime_error("Template perm table is not an object");
        }

        auto t = LandPermTable{};
        JSON::jsonToStructTryPatch(json, t); // 反射并补丁

        mLandTemplatePermTable = std::make_unique<LandTemplatePermTable>(t);
    } catch (...) {
        mLandTemplatePermTable = std::make_unique<LandTemplatePermTable>(LandPermTable{});
        PLand::getInstance().getSelf().getLogger().error(
            "Failed to load template perm table, using default perm table instead"
        );
    }
}

void LandRegistry::_buildDimensionChunkMap() {
    for (auto& [id, land] : mLandCache) {
        mDimensionChunkMap.addLand(land);
    }
}

LandID LandRegistry::getNextLandID() const { return mLandIdAllocator->nextId(); }

Result<void, StorageLayerError::Error> LandRegistry::_removeLand(SharedLand const& ptr) {
    mDimensionChunkMap.removeLand(ptr);
    if (!mLandCache.erase(ptr->getId())) {
        mDimensionChunkMap.addLand(ptr);
        return std::unexpected(StorageLayerError::Error::STLMapError);
    }

    if (!this->mDB->del(std::to_string(ptr->getId()))) {
        mLandCache.emplace(ptr->getId(), ptr); // rollback
        mDimensionChunkMap.addLand(ptr);
        return std::unexpected(StorageLayerError::Error::DBError);
    }
    return {};
}

} // namespace land


namespace land {

void LandRegistry::save() {
    std::shared_lock<std::shared_mutex> lock(mMutex); // 获取锁
    mDB->set(DbOperatorDataKey, JSON::stringify(JSON::structTojson(mLandOperators)));

    mDB->set(DbPlayerSettingDataKey, JSON::stringify(JSON::structTojson(mPlayerSettings)));

    if (mLandTemplatePermTable->mDirtyCounter.isDirty()) {
        if (mDB->set(DbTemplatePermKey, JSON::structTojson(mLandTemplatePermTable->mTemplatePermTable).dump())) {
            mLandTemplatePermTable->mDirtyCounter.reset();
        }
    }

    for (auto const& land : mLandCache | std::views::values) {
        land->save();
    }
}

bool LandRegistry::save(Land const& land) const { return mDB->set(std::to_string(land.getId()), land.dump().dump()); }

LandRegistry::LandRegistry() {
    auto& logger = land::PLand::getInstance().getSelf().getLogger();

    logger.trace("打开数据库...");
    _connectDatabaseAndCheckVersion();

    auto lock = std::unique_lock<std::shared_mutex>(mMutex);
    logger.trace("加载操作员...");
    _loadOperators();
    logger.info("已加载 {} 位操作员", mLandOperators.size());

    logger.trace("加载玩家设置...");
    _loadPlayerSettings();
    logger.info("已加载 {} 位玩家的设置", mPlayerSettings.size());

    logger.trace("加载领地数据...");
    _loadLands();
    logger.info("已加载 {} 块领地数据", mLandCache.size());

    logger.trace("加载模板权限表...");
    _loadLandTemplatePermTable();
    logger.info("已加载模板权限表");

    logger.trace("构建维度区块映射...");
    _buildDimensionChunkMap();
    logger.info("初始化维度区块映射完成");

    lock.unlock();
    mThread = std::thread([this]() {
        static std::time_t lastSaveTime = std::time(nullptr);
        while (!mThreadStopFlag) {
            std::this_thread::sleep_for(std::chrono::seconds(5)); // 5秒检查一次 & 2分钟保存一次
            if (std::time(nullptr) - lastSaveTime < 120) continue;
            lastSaveTime = std::time(nullptr); // 更新时间

            if (!mThreadStopFlag) {
                land::PLand::getInstance().getSelf().getLogger().debug("[Thread] Saving land data...");
                this->save();
                land::PLand::getInstance().getSelf().getLogger().debug("[Thread] Land data saved.");
            } else break;
        }
    });
}

LandRegistry::~LandRegistry() {
    mThreadStopFlag = true;
    if (mThread.joinable()) mThread.join();
}

bool LandRegistry::isOperator(UUIDs const& uuid) const {
    if (uuid.empty()) return false;
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return std::find(mLandOperators.begin(), mLandOperators.end(), uuid) != mLandOperators.end();
}
bool LandRegistry::addOperator(UUIDs const& uuid) {
    if (isOperator(uuid)) {
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁
    mLandOperators.push_back(uuid);
    return true;
}
bool LandRegistry::removeOperator(UUIDs const& uuid) {
    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁

    auto iter = std::find(mLandOperators.begin(), mLandOperators.end(), uuid);
    if (iter == mLandOperators.end()) {
        return false;
    }
    mLandOperators.erase(iter);
    return true;
}
std::vector<UUIDs> const& LandRegistry::getOperators() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mLandOperators;
}


PlayerSettings* LandRegistry::getPlayerSettings(UUIDs const& uuid) {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    auto                                iter = mPlayerSettings.find(uuid);
    if (iter == mPlayerSettings.end()) {
        return nullptr;
    }
    return &iter->second;
}
bool LandRegistry::setPlayerSettings(UUIDs const& uuid, PlayerSettings settings) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mPlayerSettings[uuid] = std::move(settings);
    return true;
}
bool LandRegistry::hasPlayerSettings(UUIDs const& uuid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mPlayerSettings.find(uuid) != mPlayerSettings.end();
}

LandTemplatePermTable& LandRegistry::getLandTemplatePermTable() const { return *mLandTemplatePermTable; }

bool LandRegistry::hasLand(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mLandCache.find(id) != mLandCache.end();
}
Result<void, StorageLayerError::Error> LandRegistry::_addLand(SharedLand land) {
    if (!land || land->getId() != LandID(-1)) {
        return std::unexpected(StorageLayerError::Error::InvalidLand);
    }

    LandID id = getNextLandID();
    if (hasLand(id)) {
        for (size_t i = 0; i < 3; i++) {
            id = getNextLandID();
            if (!hasLand(id)) {
                break;
            }
        }
        if (hasLand(id)) {
            return std::unexpected(StorageLayerError::Error::AssignLandIdFailed);
        }
    }
    land->mContext.mLandID = id;
    land->mDirtyCounter.increment();

    std::unique_lock<std::shared_mutex> lock(mMutex);

    auto result = mLandCache.emplace(land->getId(), land);
    if (!result.second) {
        land::PLand::getInstance().getSelf().getLogger().warn("添加领地失败, ID: {}", land->getId());
        return std::unexpected(StorageLayerError::Error::STLMapError);
    }

    mDimensionChunkMap.addLand(land);

    return {};
}
void LandRegistry::refreshLandRange(SharedLand const& ptr) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mDimensionChunkMap.refreshRange(ptr);
}

Result<void, StorageLayerError::Error> LandRegistry::addOrdinaryLand(SharedLand const& land) {
    if (!land->isOrdinaryLand()) {
        return std::unexpected(StorageLayerError::Error::LandTypeWithRequireTypeNotMatch);
    }
    if (!LandCreateValidator::isLandRangeLegal(land->getAABB(), land->getDimensionId(), land->is3D())
        || !LandCreateValidator::isLandInForbiddenRange(land->getAABB(), land->getDimensionId())
        || !LandCreateValidator::isLandRangeWithOtherCollision(this, land)) {
        return std::unexpected(StorageLayerError::Error::LandRangeIllegal);
    }
    return _addLand(land);
}

Result<void, StorageLayerError::Error> LandRegistry::addSubLand(SharedLand const& parent, SharedLand const& sub) {
    if (!LandCreateValidator::isLandRangeLegal(sub->getAABB(), parent->getDimensionId(), true)
        || !LandCreateValidator::isSubLandPositionLegal(parent, sub->getAABB())
        || parent->getDimensionId() != sub->getDimensionId()) {
        return std::unexpected(StorageLayerError::Error::LandRangeIllegal);
    }
    auto res = _addLand(sub);
    if (!res) {
        return res;
    }
    std::unique_lock<std::shared_mutex> lock(mMutex);
    parent->mContext.mSubLandIDs.push_back(sub->getId());
    sub->mContext.mParentLandID = parent->getId();
    parent->mDirtyCounter.increment();
    sub->mDirtyCounter.increment();
    return {};
}


// 加锁方法
bool LandRegistry::removeLand(LandID landId) {
    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁

    auto landIter = mLandCache.find(landId);
    if (landIter == mLandCache.end()) {
        return false;
    }
    lock.unlock();

    auto result = removeOrdinaryLand(landIter->second);
    if (!result.has_value()) {
        return false; // 移除失败
    }
    return true;
}
Result<void, StorageLayerError::Error> LandRegistry::removeOrdinaryLand(SharedLand const& ptr) {
    if (!ptr->isOrdinaryLand()) {
        return std::unexpected(StorageLayerError::Error::LandTypeWithRequireTypeNotMatch);
    }

    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁
    return _removeLand(ptr);
}
Result<void, StorageLayerError::Error> LandRegistry::removeSubLand(SharedLand const& ptr) {
    if (!ptr->isSubLand()) {
        return std::unexpected(StorageLayerError::Error::LandTypeWithRequireTypeNotMatch);
    }

    auto parent = ptr->getParentLand();
    if (!parent) {
        return std::unexpected(StorageLayerError::Error::DataConsistencyError);
    }

    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁

    // 移除父领地中的记录
    std::erase_if(parent->mContext.mSubLandIDs, [&](LandID const& id) { return id == ptr->getId(); });
    parent->mDirtyCounter.increment();

    auto result = _removeLand(ptr);
    if (!result.has_value()) {
        parent->mContext.mSubLandIDs.push_back(ptr->getId()); // 恢复父领地的子领地列表
        parent->mDirtyCounter.decrement();
    }

    return result;
}
Result<void, StorageLayerError::Error> LandRegistry::removeLandAndSubLands(SharedLand const& ptr) {
    if (!ptr->isParentLand() && !ptr->isMixLand()) {
        return std::unexpected(StorageLayerError::Error::LandTypeWithRequireTypeNotMatch);
    }

    auto currentId = ptr->getId();
    auto parent    = ptr->getParentLand();
    if (parent) {
        std::erase_if(parent->mContext.mSubLandIDs, [&](LandID const& id) { return id == currentId; });
        parent->mDirtyCounter.increment();
    }

    std::unique_lock<std::shared_mutex> lock(mMutex);
    std::stack<SharedLand>              stack;        // 栈
    std::vector<SharedLand>             removedLands; // 已移除的领地

    stack.push(ptr);

    while (!stack.empty()) {
        auto current = stack.top();
        stack.pop();

        if (current->hasSubLand()) {
            lock.unlock();
            auto subLands = current->getSubLands();
            lock.lock();
            for (auto& subLand : subLands) {
                stack.push(subLand);
            }
        }

        auto result = _removeLand(current);
        if (result.has_value()) {
            removedLands.push_back(current);
        } else {
            // rollback
            for (auto land : removedLands) {
                mLandCache.emplace(land->getId(), land);
                mDimensionChunkMap.addLand(land);
            }
            if (parent) {
                parent->mContext.mSubLandIDs.push_back(currentId); // 恢复父领地的子领地列表
                parent->mDirtyCounter.decrement();
            }
            // return std::unexpected("remove land or sub land failed!");
            return result;
        }
    }
    return {};
}
Result<void, StorageLayerError::Error> LandRegistry::removeLandAndPromoteSubLands(SharedLand const& ptr) {
    if (!ptr->isParentLand()) {
        return std::unexpected(StorageLayerError::Error::LandTypeWithRequireTypeNotMatch);
    }


    auto subLands = ptr->getSubLands();

    std::unique_lock<std::shared_mutex> lock(mMutex);
    for (auto& subLand : subLands) {
        static const auto invalidID     = LandID(-1); // 无效ID
        subLand->mContext.mParentLandID = invalidID;
        subLand->mDirtyCounter.increment();
    }

    auto result = _removeLand(ptr);
    if (!result.has_value()) {
        // rollback
        auto currentId = ptr->getId();
        for (auto& subLand : subLands) {
            subLand->mContext.mParentLandID = currentId;
            subLand->mDirtyCounter.decrement();
        }
    }
    return result;
}
Result<void, StorageLayerError::Error> LandRegistry::removeLandAndTransferSubLands(SharedLand const& ptr) {
    if (!ptr->isMixLand()) {
        return std::unexpected(StorageLayerError::Error::LandTypeWithRequireTypeNotMatch);
    }

    auto parent = ptr->getParentLand();
    if (!parent) {
        return std::unexpected(StorageLayerError::Error::DataConsistencyError);
    }
    auto parentID = parent->getId();
    auto subLands = ptr->getSubLands();

    std::unique_lock<std::shared_mutex> lock(mMutex);

    for (auto& subLand : subLands) {
        subLand->mContext.mParentLandID = parentID;               // 当前领地的子领地移交给父领地
        parent->mContext.mSubLandIDs.push_back(subLand->getId()); // 父领地记录中添加当前领地的子领地
        subLand->mDirtyCounter.increment();
        parent->mDirtyCounter.increment();
    }

    // 父领地记录中擦粗当前领地
    std::erase_if(parent->mContext.mSubLandIDs, [&](LandID const& id) { return id == ptr->getId(); });
    parent->mDirtyCounter.increment();

    auto result = _removeLand(ptr);
    if (!result.has_value()) {
        // rollback
        auto currentId = ptr->getId();
        for (auto& subLand : subLands) {
            subLand->mContext.mParentLandID = currentId;
            std::erase_if(parent->mContext.mSubLandIDs, [&](LandID const& id) { return id == subLand->getId(); });
            subLand->mDirtyCounter.decrement();
            parent->mDirtyCounter.decrement();
        }
        parent->mContext.mSubLandIDs.push_back(currentId); // 恢复父领地的子领地列表
        parent->mDirtyCounter.decrement();
    }

    return result;
}


WeakLand LandRegistry::getLandWeakPtr(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    auto landIt = mLandCache.find(id);
    if (landIt != mLandCache.end()) {
        return {landIt->second};
    }
    return {}; // 返回一个空的weak_ptr
}
SharedLand LandRegistry::getLand(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    auto landIt = mLandCache.find(id);
    if (landIt != mLandCache.end()) {
        return landIt->second;
    }
    return nullptr;
}
std::vector<SharedLand> LandRegistry::getLands() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<SharedLand> lands;
    lands.reserve(mLandCache.size());
    for (auto& land : mLandCache) {
        lands.push_back(land.second);
    }
    return lands;
}
std::vector<SharedLand> LandRegistry::getLands(std::vector<LandID> const& ids) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<SharedLand> lands;
    for (auto id : ids) {
        if (auto iter = mLandCache.find(id); iter != mLandCache.end()) {
            lands.push_back(iter->second);
        }
    }
    return lands;
}
std::vector<SharedLand> LandRegistry::getLands(LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<SharedLand> lands;
    for (auto& land : mLandCache) {
        if (land.second->getDimensionId() == dimid) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::vector<SharedLand> LandRegistry::getLands(UUIDs const& uuid, bool includeShared) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<SharedLand> lands;
    for (auto& land : mLandCache) {
        if (land.second->isOwner(uuid) || (includeShared && land.second->isMember(uuid))) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::vector<SharedLand> LandRegistry::getLands(UUIDs const& uuid, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<SharedLand> lands;
    for (auto& land : mLandCache) {
        if (land.second->getDimensionId() == dimid && land.second->isOwner(uuid)) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::unordered_map<UUIDs, std::unordered_set<SharedLand>> LandRegistry::getLandsByOwner() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::unordered_map<UUIDs, std::unordered_set<SharedLand>> lands;
    for (const auto& ptr : mLandCache | std::views::values) {
        auto& owner = ptr->getOwner();
        lands[owner].insert(ptr);
    }
    return lands;
}
std::unordered_map<UUIDs, std::unordered_set<SharedLand>> LandRegistry::getLandsByOwner(LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::unordered_map<UUIDs, std::unordered_set<SharedLand>> res;
    for (const auto& ptr : mLandCache | std::views::values) {
        if (ptr->getDimensionId() != dimid) {
            continue;
        }
        auto& owner = ptr->getOwner();
        res[owner].insert(ptr);
    }
    return res;
}


LandPermType LandRegistry::getPermType(UUIDs const& uuid, LandID id, bool ignoreOperator) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    if (!ignoreOperator && isOperator(uuid)) return LandPermType::Operator;

    if (auto land = getLand(id); land) {
        return land->getPermType(uuid);
    }

    return LandPermType::Guest;
}


SharedLand LandRegistry::getLandAt(BlockPos const& pos, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    std::unordered_set<SharedLand>      result;

    auto landsIds = mDimensionChunkMap.queryLand(dimid, EncodeChunkID(pos.x >> 4, pos.z >> 4));
    if (!landsIds) {
        return nullptr;
    }

    for (auto const& id : *landsIds) {
        if (auto iter = mLandCache.find(id); iter != mLandCache.end()) {
            if (auto const& land = iter->second; land->getAABB().hasPos(pos, !land->is3D())) {
                result.insert(land);
            }
        }
    }

    if (!result.empty()) {
        if (result.size() == 1) {
            return *result.begin(); // 只有一个领地，即普通领地
        }

        // 子领地优先级最高
        SharedLand deepestLand = nullptr;
        int        maxLevel    = -1;
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
std::unordered_set<SharedLand> LandRegistry::getLandAt(BlockPos const& center, int radius, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    if (!mDimensionChunkMap.hasDimension(dimid)) {
        return {};
    }

    std::unordered_set<ChunkID>    visitedChunks; // 记录已访问的区块
    std::unordered_set<SharedLand> lands;

    int minChunkX = (center.x - radius) >> 4;
    int minChunkZ = (center.z - radius) >> 4;
    int maxChunkX = (center.x + radius) >> 4;
    int maxChunkZ = (center.z + radius) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            ChunkID chunkId = EncodeChunkID(x, z);
            if (visitedChunks.find(chunkId) != visitedChunks.end()) {
                continue; // 如果区块已经访问过，则跳过
            }
            visitedChunks.insert(chunkId);

            auto landsIds = mDimensionChunkMap.queryLand(dimid, chunkId);
            if (!landsIds) {
                continue;
            }

            for (auto const& id : *landsIds) {
                if (auto iter = mLandCache.find(id); iter != mLandCache.end()) {
                    if (auto const& land = iter->second; land->isCollision(center, radius)) {
                        lands.insert(land);
                    }
                }
            }
        }
    }
    return lands;
}
std::unordered_set<SharedLand>
LandRegistry::getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    if (!mDimensionChunkMap.hasDimension(dimid)) {
        return {};
    }

    std::unordered_set<ChunkID>    visitedChunks;
    std::unordered_set<SharedLand> lands;

    int minChunkX = std::min(pos1.x, pos2.x) >> 4;
    int minChunkZ = std::min(pos1.z, pos2.z) >> 4;
    int maxChunkX = std::max(pos1.x, pos2.x) >> 4;
    int maxChunkZ = std::max(pos1.z, pos2.z) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            ChunkID chunkId = EncodeChunkID(x, z);
            if (visitedChunks.find(chunkId) != visitedChunks.end()) {
                continue;
            }
            visitedChunks.insert(chunkId);

            auto landsIds = mDimensionChunkMap.queryLand(dimid, chunkId);
            if (!landsIds) {
                continue;
            }

            for (auto const& id : *landsIds) {
                if (auto iter = mLandCache.find(id); iter != mLandCache.end()) {
                    if (auto const& land = iter->second; land->isCollision(pos1, pos2)) {
                        lands.insert(land);
                    }
                }
            }
        }
    }
    return lands;
}


} // namespace land


namespace land {
ChunkID LandRegistry::EncodeChunkID(int x, int z) {
    auto ux = static_cast<uint64_t>(std::abs(x));
    auto uz = static_cast<uint64_t>(std::abs(z));

    uint64_t signBits = 0;
    if (x >= 0) signBits |= (1ULL << 63);
    if (z >= 0) signBits |= (1ULL << 62);
    return signBits | (ux << 31) | (uz & 0x7FFFFFFF);
    // Memory layout:
    // [signBits][x][z] (signBits: 2 bits, x: 31 bits, z: 31 bits)
}
std::pair<int, int> LandRegistry::DecodeChunkID(ChunkID id) {
    bool xPositive = (id & (1ULL << 63)) != 0;
    bool zPositive = (id & (1ULL << 62)) != 0;

    int x = static_cast<int>((id >> 31) & 0x7FFFFFFF);
    int z = static_cast<int>(id & 0x7FFFFFFF);
    if (!xPositive) x = -x;
    if (!zPositive) z = -z;
    return {x, z};
}
} // namespace land
