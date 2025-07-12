#include "LandDimensionChunkMap.h"
#include "LandRegistry.h"
#include "pland/infra/BidirectionalMap.h"

namespace land {

LandDimensionChunkMap::LandDimensionChunkMap() = default;

bool LandDimensionChunkMap::hasDimension(LandDimid dimid) const { return mMap.contains(dimid); }

bool LandDimensionChunkMap::hasChunk(LandDimid dimid, ChunkID chunkid) const {
    if (!mMap.contains(dimid)) {
        return false;
    }
    return mMap.at(dimid).has_left(chunkid);
}

bool LandDimensionChunkMap::hasLand(LandDimid dimid, LandID landid) const {
    if (!mMap.contains(dimid)) {
        return false;
    }
    return mMap.at(dimid).has_right(landid);
}

std::unordered_set<LandID> const* LandDimensionChunkMap::queryLand(LandDimid dimId, ChunkID chunkId) const {
    if (!mMap.contains(dimId)) {
        return nullptr;
    }
    if (!mMap.at(dimId).has_left(chunkId)) {
        return nullptr;
    }
    return &mMap.at(dimId).at(chunkId);
}

std::unordered_set<ChunkID> const* LandDimensionChunkMap::queryChunk(LandDimid dimId, LandID landId) const {
    if (!mMap.contains(dimId)) {
        return nullptr;
    }
    if (!mMap.at(dimId).has_right(landId)) {
        return nullptr;
    }
    return &mMap.at(dimId).at(landId);
}

void LandDimensionChunkMap::addLand(SharedLand const& land) {
    auto landDimId = land->getDimensionId();
    auto landId    = land->getId();

    auto chunkIds = land->getAABB().getChunks()
                  | std::views::transform([](auto& c) { return LandRegistry::EncodeChunkID(c.x, c.z); });

    auto& dim = mMap[landDimId];
    for (auto chunkId : chunkIds) {
        dim.insert(chunkId, landId);
    }
}

void LandDimensionChunkMap::removeLand(SharedLand const& land) {
    auto landDimId = land->getDimensionId();
    auto landId    = land->getId();

    if (!mMap.contains(landDimId)) {
        return;
    }

    auto& dim = mMap.at(landDimId);
    for (auto chunkId : *queryChunk(landDimId, landId)) {
        dim.erase_value(chunkId, landId);
    }
}

void LandDimensionChunkMap::refreshRange(SharedLand const& land) {
    auto landDimId = land->getDimensionId();
    if (!mMap.contains(landDimId)) {
        return;
    }
    removeLand(land);
    addLand(land);
}


} // namespace land