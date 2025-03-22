#include "pland/LandPos.h"
#include "fmt/format.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Config.h"
#include <utility>

namespace land {

// PosBase
std::string PosBase::toString() const { return fmt::format("({},{},{})", x, y, z); }
bool        PosBase::isZero() const { return x == 0 && y == 0 && z == 0; }
PosBase::operator BlockPos() const { return {x, y, z}; }
PosBase::operator Vec3() const { return {x, y, z}; }
bool PosBase::operator==(const PosBase& pos) const { return this->x == pos.x && this->y == pos.y && this->z == pos.z; }
bool PosBase::operator!=(const PosBase& pos) const { return !(*this == pos); }
bool PosBase::operator==(const BlockPos& pos) const { return this->x == pos.x && this->y == pos.y && this->z == pos.z; }
bool PosBase::operator!=(const BlockPos& pos) const { return !(*this == pos); }
PosBase& PosBase::operator=(const BlockPos& pos) {
    this->x = pos.x;
    this->y = pos.y;
    this->z = pos.z;
    return *this;
}
PosBase PosBase::make(int x, int y, int z) { return PosBase{x, y, z}; }
PosBase PosBase::make(BlockPos const& pos) { return PosBase{pos.x, pos.y, pos.z}; }
int     PosBase::distance(Vec3 const& pos) const {
    float dx = pos.x - (float)x;
    float dy = pos.y - (float)y;
    float dz = pos.z - (float)z;
    return (int)std::sqrt(dx * dx + dy * dy + dz * dz);
}

// LandPos
int LandPos::getDepth() const { return mMax_B.x - mMin_A.x; }
int LandPos::getHeight() const { return mMax_B.y - mMin_A.y; }
int LandPos::getWidth() const { return mMax_B.z - mMin_A.z; }
int LandPos::getSquare() const {
    int64 depth  = getDepth();
    int64 width  = getWidth();
    int64 square = depth * width;
    if (square > INT_MAX) {
        return INT_MAX;
    }
    return square;
}
int LandPos::getVolume() const {
    int64 square = getSquare();
    int64 height = getHeight();
    int64 volume = square * height;
    if (volume > INT_MAX) {
        return INT_MAX;
    }
    return volume;
}


void LandPos::fix() {
    if (mMin_A.x > mMax_B.x) std::swap(mMin_A.x, mMax_B.x);
    if (mMin_A.y > mMax_B.y) std::swap(mMin_A.y, mMax_B.y);
    if (mMin_A.z > mMax_B.z) std::swap(mMin_A.z, mMax_B.z);
}
LandPos LandPos::make(BlockPos const& min, BlockPos const& max) {
    LandPos inst(PosBase::make(min), PosBase::make(max));
    inst.fix();
    return inst;
}
std::string LandPos::toString() const { return fmt::format("{} => {}", mMin_A.toString(), mMax_B.toString()); }
std::vector<ChunkPos> LandPos::getChunks() const {
    std::vector<ChunkPos> chunks;

    int minChunkX = mMin_A.x >> 4;
    int minChunkZ = mMin_A.z >> 4;
    int maxChunkX = mMax_B.x >> 4;
    int maxChunkZ = mMax_B.z >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            chunks.emplace_back(x, z);
        }
    }
    return chunks;
}

std::vector<BlockPos> LandPos::getBorder() const {
    std::vector<BlockPos> border;
    for (int x = mMin_A.x; x <= mMax_B.x; ++x) {
        border.emplace_back(x, mMin_A.y, mMin_A.z);
        border.emplace_back(x, mMin_A.y, mMax_B.z);
        border.emplace_back(x, mMax_B.y, mMin_A.z);
        border.emplace_back(x, mMax_B.y, mMax_B.z);
    }
    for (int y = mMin_A.y + 1; y < mMax_B.y; ++y) {
        border.emplace_back(mMin_A.x, y, mMin_A.z);
        border.emplace_back(mMin_A.x, y, mMax_B.z);
        border.emplace_back(mMax_B.x, y, mMin_A.z);
        border.emplace_back(mMax_B.x, y, mMax_B.z);
    }
    for (int z = mMin_A.z + 1; z < mMax_B.z; ++z) {
        border.emplace_back(mMin_A.x, mMin_A.y, z);
        border.emplace_back(mMin_A.x, mMax_B.y, z);
        border.emplace_back(mMax_B.x, mMin_A.y, z);
        border.emplace_back(mMax_B.x, mMax_B.y, z);
    }
    for (int y = mMin_A.y; y <= mMax_B.y; ++y) {
        border.emplace_back(mMin_A.x, y, mMin_A.z);
        border.emplace_back(mMin_A.x, y, mMax_B.z);
        border.emplace_back(mMax_B.x, y, mMin_A.z);
        border.emplace_back(mMax_B.x, y, mMax_B.z);
    }
    return border;
}

std::vector<BlockPos> LandPos::getRange() const {
    std::vector<BlockPos> range;
    for (int x = mMin_A.x; x <= mMax_B.x; ++x) {
        range.emplace_back(x, mMax_B.y, mMin_A.z);
        range.emplace_back(x, mMax_B.y, mMax_B.z);
    }
    for (int z = mMin_A.z + 1; z < mMax_B.z; ++z) {
        range.emplace_back(mMin_A.x, mMax_B.y, z);
        range.emplace_back(mMax_B.x, mMax_B.y, z);
    }
    return range;
}

bool LandPos::hasPos(const BlockPos& pos, bool ignoreY) const {
    if (ignoreY) {
        return pos.x >= mMin_A.x && pos.x <= mMax_B.x && pos.z >= mMin_A.z && pos.z <= mMax_B.z;
    } else {
        return pos.x >= mMin_A.x && pos.x <= mMax_B.x && pos.y >= mMin_A.y && pos.y <= mMax_B.y && pos.z >= mMin_A.z
            && pos.z <= mMax_B.z;
    }
}

bool LandPos::operator==(LandPos const& pos) const { return mMin_A == pos.mMin_A && mMax_B == pos.mMax_B; }
bool LandPos::operator!=(LandPos const& pos) const { return !(*this == pos); }


bool LandPos::isCollision(const LandPos& pos1, const LandPos& pos2) {
    return !(
        pos1.mMax_B.x < pos2.mMin_A.x || pos1.mMin_A.x > pos2.mMax_B.x || pos1.mMax_B.y < pos2.mMin_A.y
        || pos1.mMin_A.y > pos2.mMax_B.y || pos1.mMax_B.z < pos2.mMin_A.z || pos1.mMin_A.z > pos2.mMax_B.z
    );
}
bool LandPos::isComplisWithMinSpacing(const LandPos& pos1, const LandPos& pos2, int minSpacing) {
    // 检查 X 轴
    int xDist = std::min(std::abs(pos1.mMax_B.x - pos2.mMin_A.x), std::abs(pos2.mMax_B.x - pos1.mMin_A.x));
    if (xDist < minSpacing) return false;

    // 检查 Z 轴
    int zDist = std::min(std::abs(pos1.mMax_B.z - pos2.mMin_A.z), std::abs(pos2.mMax_B.z - pos1.mMin_A.z));
    if (zDist < minSpacing) return false;

    // 检查 Y 轴
    int yDist = std::min(std::abs(pos1.mMax_B.y - pos2.mMin_A.y), std::abs(pos2.mMax_B.y - pos1.mMin_A.y));
    if (yDist < minSpacing) return false;

    return true;
}
bool LandPos::isContain(LandPos const& src, LandPos const& dst) {
    return src.mMin_A.x <= dst.mMin_A.x && src.mMax_B.x >= dst.mMax_B.x && //
           src.mMin_A.y <= dst.mMin_A.y && src.mMax_B.y >= dst.mMax_B.y && //
           src.mMin_A.z <= dst.mMin_A.z && src.mMax_B.z >= dst.mMax_B.z;
}


} // namespace land