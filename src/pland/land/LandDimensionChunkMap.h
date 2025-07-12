#pragma once
#include "Land.h"
#include "pland/Global.h"
#include "pland/infra/BidirectionalMap.h"
#include <concepts>
#include <cstdint>
#include <unordered_map>

namespace land {


/**
 * @brief 领地维度区块双向映射表
 *         / --> 区块 --> [领地]  # 查询领地
 * 维度 --|
 *        \ --> 领地 --> [区块]   # 查询区块
 */
class LandDimensionChunkMap {
public:
    using Map = std::unordered_map<LandDimid, BidirectionalMap<ChunkID, LandID>>;

public:
    LDAPI LandDimensionChunkMap();

    /**
     * @brief 查询维度是否存在
     */
    LDNDAPI bool hasDimension(LandDimid dimId) const;

    /**
     * @brief 查询区块是否存在
     */
    LDNDAPI bool hasChunk(LandDimid dimId, ChunkID chunkId) const;

    /**
     * @brief 查询领地是否存在
     */
    LDNDAPI bool hasLand(LandDimid dimId, ChunkID chunk, LandID landId) const;

    /**
     * @brief 查询某个区块下所有的领地
     */
    LDNDAPI std::unordered_set<LandID> const& queryLand(LandDimid dimId, ChunkID chunkId) const;

    /**
     * @brief 查询某个领地下所有的区块
     */
    LDNDAPI std::unordered_set<ChunkID> const& queryChunk(LandDimid dimId, LandID landId) const;

    LDAPI void addLand(SharedLand const& land);

    LDAPI void removeLand(SharedLand const& land);

    LDAPI void refreshRange(SharedLand const& land);

private:
    Map mMap;
};

} // namespace land
