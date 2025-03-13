#include "pland/PLand.h"
#include "fmt/core.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/i18n/I18n.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mod/MyMod.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
#include "pland/utils/JSON.h"
#include "pland/utils/Utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
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

void PLand::_loadLands() {
    ll::coro::Generator<std::pair<std::string_view, std::string_view>> iter = mDB->iter();

    auto operatorKey      = DB_KEY_OPERATORS();
    auto playerSettingKey = DB_KEY_PLAYER_SETTINGS();

    for (auto [key, value] : iter) {
        if (key == operatorKey || key == playerSettingKey) continue;

        auto json = JSON::parse(value);
        auto land = LandData::make();

        JSON::jsonToStruct(json, *land);

        // 保证landID唯一
        if (mNextLandID < land->getLandID()) {
            mNextLandID.store(land->getLandID() + 1);
        }

        mLandCache.emplace(land->getLandID(), std::move(land));
    }
    auto& logger = my_mod::MyMod::getInstance().getSelf().getLogger();
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

            if (!some(chunkLandVec, land->getLandID())) {
                chunkLandVec.push_back(land->getLandID());
            }
        }
    }
    my_mod::MyMod::getInstance().getSelf().getLogger().info("初始化领地缓存系统完成");
}

void PLand::_updateLandMap(LandData_sptr const& ptr, bool add) {
    auto chunks = ptr->mPos.getChunks();
    for (auto& ch : chunks) {
        auto& chunkLands = mLandMap[ptr->mLandDimid][EncodeChunkID(ch.x, ch.z)];

        auto iter = std::find(chunkLands.begin(), chunkLands.end(), ptr->mLandID);

        if (add) {
            if (iter == chunkLands.end()) {
                chunkLands.push_back(ptr->mLandID);
            }
        } else {
            if (iter != chunkLands.end()) {
                chunkLands.erase(iter);
            }
        }
    }
}
void PLand::_refreshLandRange(LandData_sptr const& ptr) {
    _updateLandMap(ptr, false);
    _updateLandMap(ptr, true);
}
} // namespace land


namespace land {

void PLand::init() {
    auto dir = my_mod::MyMod::getInstance().getSelf().getDataDir() / DB_DIR_NAME();

    if (!mDB) {
        mDB = std::make_unique<ll::data::KeyValueDB>(dir);
    }

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
                my_mod::MyMod::getInstance().getSelf().getLogger().debug("[Thread] Saving land data...");
                this->save();
                my_mod::MyMod::getInstance().getSelf().getLogger().debug("[Thread] Land data saved.");
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
    mPlayerSettings[uuid] = settings;
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
bool PLand::addLand(LandData_sptr land) {
    if (!land || land->mLandID != LandID(-1)) {
        return false;
    }

    LandID id = generateLandID();
    if (hasLand(id)) {
        for (size_t i = 0; i < 3; i++) {
            id = generateLandID();
            if (!hasLand(id)) {
                break;
            }
        }
        if (hasLand(id)) {
            return false; // ID 冲突
        }
    }
    land->mLandID = id;

    std::unique_lock<std::shared_mutex> lock(mMutex);

    auto result = mLandCache.emplace(land->mLandID, land);
    if (!result.second) {
        my_mod::MyMod::getInstance().getSelf().getLogger().warn("添加领地失败, ID: {}", land->mLandID);
        return false;
    }

    _updateLandMap(land, true);

    return true;
}
bool PLand::removeLand(LandID landId) {
    std::unique_lock<std::shared_mutex> lock(mMutex); // 获取锁

    auto landIter = mLandCache.find(landId);
    if (landIter == mLandCache.end()) {
        return false;
    }

    _updateLandMap(landIter->second, false); // 擦除映射表

    mLandCache.erase(landIter);             // 删除缓存
    this->mDB->del(std::to_string(landId)); // 删除数据库记录

    return true;
}
void PLand::refreshLandRange(LandData_sptr const& ptr) {
    std::unique_lock<std::shared_mutex> lock(mMutex);
    _refreshLandRange(ptr);
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

LandPermType PLand::getPermType(UUIDs const& uuid, LandID id, bool ignoreOperator) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    if (!ignoreOperator && isOperator(uuid)) return LandPermType::Operator;

    if (auto land = getLand(id); land) {
        return land->getPermType(uuid);
    }

    return LandPermType::Guest;
}

LandID PLand::generateLandID() { return mNextLandID++; }

LandData_sptr PLand::getLandAt(BlockPos const& pos, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    ChunkID chunkId = EncodeChunkID(pos.x >> 4, pos.z >> 4);
    auto    dimIt   = mLandMap.find(dimid); // 查找维度
    if (dimIt != mLandMap.end()) {
        auto chunkIt = dimIt->second.find(chunkId); // 查找区块
        if (chunkIt != dimIt->second.end()) {
            for (const auto& landId : chunkIt->second) {
                auto landIt = mLandCache.find(landId); // 查找领地
                if (landIt != mLandCache.end()
                    && landIt->second->getLandPos().hasPos(pos, !landIt->second->is3DLand())) {
                    return landIt->second;
                }
            }
        }
    }
    return nullptr;
}
std::vector<LandData_sptr> PLand::getLandAt(BlockPos const& center, int radius, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<LandData_sptr> lands;

    int minChunkX = (center.x - radius) >> 4;
    int minChunkZ = (center.z - radius) >> 4;
    int maxChunkX = (center.x + radius) >> 4;
    int maxChunkZ = (center.z + radius) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            ChunkID chunkId = EncodeChunkID(x, z);
            auto    dimIt   = mLandMap.find(dimid); // 查找维度
            if (dimIt != mLandMap.end()) {
                auto chunkIt = dimIt->second.find(chunkId); // 查找区块
                if (chunkIt != dimIt->second.end()) {
                    for (const auto& landId : chunkIt->second) {
                        auto landIt = mLandCache.find(landId); // 查找领地
                        if (landIt != mLandCache.end() && landIt->second->isRadiusInLand(center, radius)) {
                            lands.push_back(landIt->second);
                        }
                    }
                }
            }
        }
    }
    return lands;
}
std::vector<LandData_sptr> PLand::getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const {
    std::shared_lock<std::shared_mutex> lock(mMutex);

    std::vector<LandData_sptr> lands;

    int minChunkX = std::min(pos1.x, pos2.x) >> 4;
    int minChunkZ = std::min(pos1.z, pos2.z) >> 4;
    int maxChunkX = std::max(pos1.x, pos2.x) >> 4;
    int maxChunkZ = std::max(pos1.z, pos2.z) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            ChunkID chunkId = EncodeChunkID(x, z);
            auto    dimIt   = mLandMap.find(dimid); // 查找维度
            if (dimIt != mLandMap.end()) {
                auto chunkIt = dimIt->second.find(chunkId); // 查找区块
                if (chunkIt != dimIt->second.end()) {
                    for (const auto& landId : chunkIt->second) {
                        auto landIt = mLandCache.find(landId); // 查找领地
                        if (landIt != mLandCache.end() && landIt->second->isAABBInLand(pos1, pos2)) {
                            lands.push_back(landIt->second);
                        }
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
} // namespace land
