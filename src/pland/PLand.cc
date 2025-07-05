#include "pland/PLand.h"
#include "fmt/core.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/i18n/I18n.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/math/LandAABB.h"
#include "pland/mod/ModEntry.h"
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


void PLand::_loadOperators() {
    if (!mDB->has(DB_KEY_OPERATORS())) {
        mDB->set(DB_KEY_OPERATORS(), "[]"); // empty array
    }
    auto ops = JSON::parse(*mDB->get(DB_KEY_OPERATORS()));
    JSON::jsonToStructNoMerge(ops, mLandOperators);
}

void PLand::_loadPlayerSettings() {
    if (!mDB->has(DB_KEY_PLAYER_SETTINGS())) {
        mDB->set(DB_KEY_PLAYER_SETTINGS(), "{}"); // empty object
    }
    auto settings = JSON::parse(*mDB->get(DB_KEY_PLAYER_SETTINGS()));
    if (!settings.is_object()) {
        throw std::runtime_error("player settings is not an object");
    }

    for (auto& [key, value] : settings.items()) {
        PlayerSettings settings_;
        JSON::jsonToStructTryPatch(value, settings_);
        mPlayerSettings.emplace(key, std::move(settings_));
    }
}

void PLand::_openDBAndCheckVersion() {
    auto&       self    = mod::ModEntry::getInstance().getSelf();
    auto&       logger  = self.getLogger();
    auto const& dataDir = self.getDataDir();
    auto const  dbDir   = dataDir / DB_DIR_NAME();

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

    auto const dbVersionKey = DB_KEY_VERSION();
    if (!mDB->has(dbVersionKey)) {
        if (isNewCreatedDB) {
            mDB->set(dbVersionKey, std::to_string(LandDataVersion)); // 设置初始版本号
        } else {
            mDB->set(dbVersionKey, "-1"); // 数据库存在，但没有版本号，表示是旧版数据库(0.8.1之前)
        }
    }

    auto version = std::stoi(*mDB->get(dbVersionKey));
    if (version != LandDataVersion) {
        if (version > LandDataVersion) {
            logger.fatal(
                "数据库版本过高，当前版本: {}, 期望版本: {}。为了保证数据安全，插件拒绝加载！",
                version,
                LandDataVersion
            );
            logger.fatal(
                "The database version is too high, current version: {}, expected version: {}. In order to "
                "keep the data safe, the plugin refuses to load!",
                version,
                LandDataVersion
            );
            throw std::runtime_error("The database versions do not match");

        } else if (version < LandDataVersion) {
            logger.warn(
                "数据库版本过低，当前版本: {}, 期望版本: {}，插件将尝试备份并升级数据库...",
                version,
                LandDataVersion
            );
            logger.warn(
                "The database version is too low, the current version: {}, the expected version: {}, the "
                "plugin will try to back up and upgrade the database...",
                version,
                LandDataVersion
            );
            mDB.reset();
            backup();
            mDB = std::make_unique<ll::data::KeyValueDB>(dbDir);
            mDB->set(dbVersionKey, std::to_string(LandDataVersion)); // 更新版本号
            // 这里只需要修改版本号以及备份，其它兼容转换操作将在 _checkAndAdaptBreakingChanges 中进行
        }
    }
}

void PLand::_checkAndAdaptBreakingChanges(nlohmann::json& landData) {
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

void PLand::_loadLands() {
    ll::coro::Generator<std::pair<std::string_view, std::string_view>> iter = mDB->iter();

    auto operatorKey      = DB_KEY_OPERATORS();
    auto playerSettingKey = DB_KEY_PLAYER_SETTINGS();
    auto versionKey       = DB_KEY_VERSION();

    for (auto [key, value] : iter) {
        if (key == operatorKey || key == playerSettingKey || key == versionKey) continue;

        auto json = JSON::parse(value);
        auto land = LandData::make();

        _checkAndAdaptBreakingChanges(json);

        JSON::jsonToStruct(json, *land);

        // 保证landID唯一
        if (mNextLandID.load() <= land->getLandID()) {
            mNextLandID.store(land->getLandID() + 1);
        }

        mLandCache.emplace(land->getLandID(), std::move(land));
    }
    auto& logger = mod::ModEntry::getInstance().getSelf().getLogger();
    logger.info("已加载 {} 位操作员", mLandOperators.size());
    logger.info("已加载 {} 块领地数据", mLandCache.size());
}

void PLand::_initLandMap() {
    for (auto& [id, land] : mLandCache) {
        auto& chunkMap = mLandMap[LandDimid(land->getLandDimid())]; // 区块表

        auto chs = land->mPos.getChunks();
        for (auto& ch : chs) {
            auto  chunkID      = PLand::EncodeChunkID(ch.x, ch.z);
            auto& chunkLandVec = chunkMap[chunkID]; // 区块领地数组

            chunkLandVec.insert(land->getLandID());
        }
    }
    mod::ModEntry::getInstance().getSelf().getLogger().info("初始化领地缓存系统完成");
}

void PLand::_updateLandMap(LandData_sptr const& ptr, bool add) {
    auto chunks = ptr->mPos.getChunks();
    for (auto& ch : chunks) {
        auto& chunkLands = mLandMap[ptr->mLandDimid][EncodeChunkID(ch.x, ch.z)];

        if (add) {
            chunkLands.insert(ptr->getLandID());
        } else {
            chunkLands.erase(ptr->getLandID());
        }
    }
}
void PLand::_refreshLandRange(LandData_sptr const& ptr) {
    _updateLandMap(ptr, false);
    _updateLandMap(ptr, true);
}

LandID PLand::getNextLandID() { return mNextLandID++; }

Result<bool> PLand::_removeLand(LandData_sptr const& ptr) {
    _updateLandMap(ptr, false); // 擦除映射表
    if (!mLandCache.erase(ptr->getLandID())) {
        return std::unexpected("erase cache failed!");
    }

    if (!this->mDB->del(std::to_string(ptr->getLandID()))) {
        mLandCache.emplace(ptr->getLandID(), ptr); // rollback
        _updateLandMap(ptr, true);
        return std::unexpected("del db failed!");
    }
    return true;
}

} // namespace land


namespace land {

void PLand::init() {
    _openDBAndCheckVersion();

    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁

    _loadOperators();

    _loadPlayerSettings();

    _loadLands();

    _initLandMap();

    lock.unlock();
    mThread = std::thread([this]() {
        static std::time_t lastSaveTime = std::time(nullptr);
        while (!mThreadStopFlag) {
            std::this_thread::sleep_for(std::chrono::seconds(5)); // 5秒检查一次 & 2分钟保存一次
            if (std::time(nullptr) - lastSaveTime < 120) continue;
            lastSaveTime = std::time(nullptr); // 更新时间

            if (!mThreadStopFlag) {
                mod::ModEntry::getInstance().getSelf().getLogger().debug("[Thread] Saving land data...");
                this->save();
                mod::ModEntry::getInstance().getSelf().getLogger().debug("[Thread] Land data saved.");
            } else break;
        }
    });
}
void PLand::save() {
    std::shared_lock<std::shared_mutex> lock(mMutex); // 获取锁
    mDB->set(DB_KEY_OPERATORS(), JSON::stringify(JSON::structTojson(mLandOperators)));

    mDB->set(DB_KEY_PLAYER_SETTINGS(), JSON::stringify(JSON::structTojson(mPlayerSettings)));

    for (auto& [id, land] : mLandCache) {
        mDB->set(std::to_string(land->mLandID), JSON::stringify(JSON::structTojson(*land)));
    }
}
void PLand::stopThread() {
    mThreadStopFlag = true;
    if (mThread.joinable()) mThread.join();
}


PLand& PLand::getInstance() {
    static PLand instance;
    return instance;
}


bool PLand::isOperator(UUIDs const& uuid) const {
    if (uuid.empty()) return false;
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return std::find(mLandOperators.begin(), mLandOperators.end(), uuid) != mLandOperators.end();
}
bool PLand::addOperator(UUIDs const& uuid) {
    if (isOperator(uuid)) {
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁
    mLandOperators.push_back(uuid);
    return true;
}
bool PLand::removeOperator(UUIDs const& uuid) {
    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁

    auto iter = std::find(mLandOperators.begin(), mLandOperators.end(), uuid);
    if (iter == mLandOperators.end()) {
        return false;
    }
    mLandOperators.erase(iter);
    return true;
}
std::vector<UUIDs> const& PLand::getOperators() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mLandOperators;
}


PlayerSettings* PLand::getPlayerSettings(UUIDs const& uuid) {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    auto                                iter = mPlayerSettings.find(uuid);
    if (iter == mPlayerSettings.end()) {
        return nullptr;
    }
    return &iter->second;
}
bool PLand::setPlayerSettings(UUIDs const& uuid, PlayerSettings settings) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    mPlayerSettings[uuid] = std::move(settings);
    return true;
}
bool PLand::hasPlayerSettings(UUIDs const& uuid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mPlayerSettings.find(uuid) != mPlayerSettings.end();
}


bool PLand::hasLand(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);
    return mLandCache.find(id) != mLandCache.end();
}
Result<bool> PLand::addLand(LandData_sptr land) {
    if (!land || land->mLandID != LandID(-1)) {
        return std::unexpected("领地数据无效或已分配ID");
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
            return std::unexpected("无法分配新的领地ID，ID冲突"); // ID 冲突
        }
    }
    land->mLandID = id;

    std::unique_lock<std::shared_mutex> lock(mMutex);

    auto result = mLandCache.emplace(land->mLandID, land);
    if (!result.second) {
        mod::ModEntry::getInstance().getSelf().getLogger().warn("添加领地失败, ID: {}", land->mLandID);
        return std::unexpected("添加领地到缓存失败");
    }

    _updateLandMap(land, true);

    return true;
}
void PLand::refreshLandRange(LandData_sptr const& ptr) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    _refreshLandRange(ptr);
}


// 加锁方法
bool PLand::removeLand(LandID landId) {
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
Result<bool> PLand::removeOrdinaryLand(LandData_sptr const& ptr) {
    if (!ptr->isOrdinaryLand()) {
        return std::unexpected("not a ordinary land!");
    }

    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁
    return _removeLand(ptr);
}
Result<bool> PLand::removeSubLand(LandData_sptr const& ptr) {
    if (!ptr->isSubLand()) {
        return std::unexpected("not a sub land!");
    }

    auto parent = ptr->getParentLand();
    if (!parent) {
        return std::unexpected("parent land not found!");
    }

    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁

    // 移除父领地中的记录
    std::erase_if(parent->mSubLandIDs, [&](LandID const& id) { return id == ptr->getLandID(); });

    auto result = _removeLand(ptr);
    if (!result.has_value()) {
        parent->mSubLandIDs.push_back(ptr->getLandID()); // 恢复父领地的子领地列表
    }

    return result;
}
Result<bool> PLand::removeLandAndSubLands(LandData_sptr const& ptr) {
    if (!ptr->isParentLand() && !ptr->isMixLand()) {
        return std::unexpected("only parent land and mix land can remove sub lands!");
    }

    auto currentId = ptr->getLandID();
    auto parent    = ptr->getParentLand();
    if (parent) {
        std::erase_if(parent->mSubLandIDs, [&](LandID const& id) { return id == currentId; });
    }

    std::unique_lock<std::shared_mutex> lock(mMutex);
    std::stack<LandData_sptr>           stack;        // 栈
    std::vector<LandData_sptr>          removedLands; // 已移除的领地

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
                mLandCache.emplace(land->getLandID(), land);
                _updateLandMap(land, true);
            }
            if (parent) {
                parent->mSubLandIDs.push_back(currentId); // 恢复父领地的子领地列表
            }
            return std::unexpected("remove land or sub land failed!");
        }
    }
    return true;
}
Result<bool> PLand::removeLandAndPromoteSubLands(LandData_sptr const& ptr) {
    if (!ptr->isParentLand()) {
        return std::unexpected("only root land's sub land can be promoted!");
    }


    auto subLands = ptr->getSubLands();

    std::unique_lock<std::shared_mutex> lock(mMutex);
    for (auto& subLand : subLands) {
        static const auto invalidID = LandID(-1); // 无效ID
        subLand->mParentLandID      = invalidID;
    }

    auto result = _removeLand(ptr);
    if (!result.has_value()) {
        // rollback
        auto currentId = ptr->getLandID();
        for (auto& subLand : subLands) {
            subLand->mParentLandID = currentId;
        }
    }
    return result;
}
Result<bool> PLand::removeLandAndTransferSubLands(LandData_sptr const& ptr) {
    if (!ptr->isMixLand()) {
        return std::unexpected("only mix land's sub land can be transferred!");
    }

    auto parent = ptr->getParentLand();
    if (!parent) {
        return std::unexpected("parent land not found!");
    }
    auto parentID = parent->getLandID();
    auto subLands = ptr->getSubLands();

    std::unique_lock<std::shared_mutex> lock(mMutex);

    for (auto& subLand : subLands) {
        subLand->mParentLandID = parentID;                   // 当前领地的子领地移交给父领地
        parent->mSubLandIDs.push_back(subLand->getLandID()); // 父领地记录中添加当前领地的子领地
    }

    // 父领地记录中擦粗当前领地
    std::erase_if(parent->mSubLandIDs, [&](LandID const& id) { return id == ptr->getLandID(); });

    auto result = _removeLand(ptr);
    if (!result.has_value()) {
        // rollback
        auto currentId = ptr->getLandID();
        for (auto& subLand : subLands) {
            subLand->mParentLandID = currentId;
            std::erase_if(parent->mSubLandIDs, [&](LandID const& id) { return id == subLand->getLandID(); });
        }
        parent->mSubLandIDs.push_back(currentId); // 恢复父领地的子领地列表
    }
    return result;
}


LandData_wptr PLand::getLandWeakPtr(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    auto landIt = mLandCache.find(id);
    if (landIt != mLandCache.end()) {
        return {landIt->second};
    }
    return {}; // 返回一个空的weak_ptr
}
LandData_sptr PLand::getLand(LandID id) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    auto landIt = mLandCache.find(id);
    if (landIt != mLandCache.end()) {
        return landIt->second;
    }
    return nullptr;
}
std::vector<LandData_sptr> PLand::getLands() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<LandData_sptr> lands;
    lands.reserve(mLandCache.size());
    for (auto& land : mLandCache) {
        lands.push_back(land.second);
    }
    return lands;
}
std::vector<LandData_sptr> PLand::getLands(std::vector<LandID> const& ids) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<LandData_sptr> lands;
    for (auto id : ids) {
        if (auto iter = mLandCache.find(id); iter != mLandCache.end()) {
            lands.push_back(iter->second);
        }
    }
    return lands;
}
std::vector<LandData_sptr> PLand::getLands(LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<LandData_sptr> lands;
    for (auto& land : mLandCache) {
        if (land.second->mLandDimid == dimid) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::vector<LandData_sptr> PLand::getLands(UUIDs const& uuid, bool includeShared) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<LandData_sptr> lands;
    for (auto& land : mLandCache) {
        if (land.second->isLandOwner(uuid) || (includeShared && land.second->isLandMember(uuid))) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::vector<LandData_sptr> PLand::getLands(UUIDs const& uuid, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<LandData_sptr> lands;
    for (auto& land : mLandCache) {
        if (land.second->mLandDimid == dimid && land.second->isLandOwner(uuid)) {
            lands.push_back(land.second);
        }
    }
    return lands;
}
std::unordered_map<UUIDs, std::unordered_set<LandData_sptr>> PLand::getLandsByOwner() const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::unordered_map<UUIDs, std::unordered_set<LandData_sptr>> lands;
    for (const auto& ptr : mLandCache | std::views::values) {
        auto& owner = ptr->getLandOwner();
        lands[owner].insert(ptr);
    }
    return lands;
}
std::unordered_map<UUIDs, std::unordered_set<LandData_sptr>> PLand::getLandsByOwner(LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::unordered_map<UUIDs, std::unordered_set<LandData_sptr>> res;
    for (const auto& ptr : mLandCache | std::views::values) {
        if (ptr->getLandDimid() != dimid) {
            continue;
        }
        auto& owner = ptr->getLandOwner();
        res[owner].insert(ptr);
    }
    return res;
}


LandPermType PLand::getPermType(UUIDs const& uuid, LandID id, bool ignoreOperator) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    if (!ignoreOperator && isOperator(uuid)) return LandPermType::Operator;

    if (auto land = getLand(id); land) {
        return land->getPermType(uuid);
    }

    return LandPermType::Guest;
}


LandData_sptr PLand::getLandAt(BlockPos const& pos, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::unordered_set<LandData_sptr> result;

    ChunkID chunkId = EncodeChunkID(pos.x >> 4, pos.z >> 4);
    auto    dimIt   = mLandMap.find(dimid); // 查找维度
    if (dimIt != mLandMap.end()) {
        auto chunkIt = dimIt->second.find(chunkId); // 查找区块
        if (chunkIt != dimIt->second.end()) {
            for (const auto& landId : chunkIt->second) {
                auto landIt = mLandCache.find(landId); // 查找领地
                if (landIt != mLandCache.end()
                    && landIt->second->getLandPos().hasPos(pos, !landIt->second->is3DLand())) {
                    // return landIt->second;
                    result.insert(landIt->second);
                }
            }
        }
    }

    if (!result.empty()) {
        if (result.size() == 1) {
            return *result.begin(); // 只有一个领地，即普通领地
        }

        // 子领地优先级最高
        LandData_sptr deepestLand = nullptr;
        int           maxLevel    = -1;
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
std::unordered_set<LandData_sptr> PLand::getLandAt(BlockPos const& center, int radius, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    auto dimIter = mLandMap.find(dimid); // 查找维度
    if (dimIter == mLandMap.end()) {
        return {};
    }

    auto&                             dim = dimIter->second;
    std::unordered_set<ChunkID>       visitedChunks; // 记录已访问的区块
    std::unordered_set<LandData_sptr> lands;

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

            auto chunkIt = dim.find(chunkId); // 查找区块
            if (chunkIt != dim.end()) {
                for (const auto& landId : chunkIt->second) {
                    auto landIt = mLandCache.find(landId); // 查找领地
                    if (landIt != mLandCache.end() && landIt->second->isRadiusInLand(center, radius)) {
                        lands.insert(landIt->second);
                    }
                }
            }
        }
    }
    return lands;
}
std::unordered_set<LandData_sptr> PLand::getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    auto dimIter = mLandMap.find(dimid); // 查找维度
    if (dimIter == mLandMap.end()) {
        return {};
    }

    auto&                             dim = dimIter->second;
    std::unordered_set<ChunkID>       visitedChunks;
    std::unordered_set<LandData_sptr> lands;

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

            auto chunkIt = dim.find(chunkId); // 查找区块
            if (chunkIt != dim.end()) {
                for (const auto& landId : chunkIt->second) {
                    auto landIt = mLandCache.find(landId); // 查找领地
                    if (landIt != mLandCache.end() && landIt->second->isAABBInLand(pos1, pos2)) {
                        lands.insert(landIt->second);
                    }
                }
            }
        }
    }
    return lands;
}


} // namespace land


namespace land {
ChunkID PLand::EncodeChunkID(int x, int z) {
    auto ux = static_cast<uint64_t>(std::abs(x));
    auto uz = static_cast<uint64_t>(std::abs(z));

    uint64_t signBits = 0;
    if (x >= 0) signBits |= (1ULL << 63);
    if (z >= 0) signBits |= (1ULL << 62);
    return signBits | (ux << 31) | (uz & 0x7FFFFFFF);
    // Memory layout:
    // [signBits][x][z] (signBits: 2 bits, x: 31 bits, z: 31 bits)
}
std::pair<int, int> PLand::DecodeChunkID(ChunkID id) {
    bool xPositive = (id & (1ULL << 63)) != 0;
    bool zPositive = (id & (1ULL << 62)) != 0;

    int x = static_cast<int>((id >> 31) & 0x7FFFFFFF);
    int z = static_cast<int>(id & 0x7FFFFFFF);
    if (!xPositive) x = -x;
    if (!zPositive) z = -z;
    return {x, z};
}

string PLand::DB_DIR_NAME() { return "db"; }
string PLand::DB_KEY_OPERATORS() { return "operators"; }
string PLand::DB_KEY_PLAYER_SETTINGS() { return "player_settings"; }
string PLand::DB_KEY_VERSION() { return "__version__"; }
} // namespace land
