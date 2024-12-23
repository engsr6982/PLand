#pragma once
#include "ll/api/data/KeyValueDB.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>


namespace land {


class PLand {
public:
    PLand()                        = default;
    PLand(const PLand&)            = delete;
    PLand& operator=(const PLand&) = delete;

private:
    std::unique_ptr<ll::data::KeyValueDB> mDB; // private

    //                 维度                         区块            领地
    std::unordered_map<LandDimid, std::unordered_map<ChunkID, std::vector<LandID>>> mLandMap;   // 领地映射表
    std::unordered_map<LandID, LandData_sptr>                                       mLandCache; // 领地缓存
    std::atomic<LandID>                                                             mNextID{0};

    std::vector<UUIDs>        mLandOperators; // 领地操作员
    mutable std::shared_mutex mMutex;         // 领地缓存锁

public:
    LDAPI static PLand& getInstance();

    LDAPI bool init();
    LDAPI bool save();
    LDAPI bool _initCache(); // private

    std::jthread mThread;       // private
    LDAPI void   _initThread(); // private

    LDAPI bool isOperator(UUIDs const& uuid) const;
    LDAPI bool addOperator(UUIDs const& uuid);
    LDAPI bool removeOperator(UUIDs const& uuid);

    LDAPI bool hasLand(LandID id) const;
    LDAPI bool addLand(LandData_sptr land);
    LDAPI bool removeLand(LandID id);

    LDAPI bool refreshLandRange(LandData_sptr ptr); // 刷新领地范围

    LDAPI LandData_wptr getLandWeakPtr(LandID id) const;                // 获取领地弱引用 (推荐)
    LDAPI LandData_sptr getLand(LandID id) const;                       // 获取领地数据
    LDAPI std::vector<LandData_sptr> getLands() const;                  // 获取所有领地数据
    LDAPI std::vector<LandData_sptr> getLands(LandDimid dimid) const;   // 获取维度领地数据
    LDAPI std::vector<LandData_sptr> getLands(UUIDs const& uuid) const; // 获取玩家领地数据
    LDAPI std::vector<LandData_sptr> getLands(UUIDs const& uuid, LandDimid dimid) const; // 获取玩家维度领地数据

    LDAPI LandPermType
    getPermType(UUIDs const& uuid, LandID id = 0, bool ignoreOperator = false) const; // 获取领地权限类型

    // 获取领地数据
    LDAPI LandData_sptr getLandAt(BlockPos const& pos, LandDimid dimid) const;
    // 半径内的领地
    LDAPI std::vector<LandData_sptr> getLandAt(BlockPos const& center, int radius, LandDimid dimid) const;
    // 矩形内的领地
    LDAPI std::vector<LandData_sptr> getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const;

    LDAPI LandID generateLandID();

    LDAPI static ChunkID             getChunkID(int x, int z);
    LDAPI static std::pair<int, int> parseChunkID(ChunkID id);
};


} // namespace land