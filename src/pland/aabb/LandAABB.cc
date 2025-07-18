#include "pland/aabb/LandAABB.h"


namespace land {

LandPos const& LandAABB::getMin() const { return min; }
LandPos const& LandAABB::getMax() const { return max; }

int   LandAABB::getDepth() const { return max.x - min.x; }
int   LandAABB::getHeight() const { return max.y - min.y; }
int   LandAABB::getWidth() const { return max.z - min.z; }
llong LandAABB::getSquare() const {
    llong depth  = getDepth();
    llong width  = getWidth();
    llong square = depth * width;
    return square;
}
llong LandAABB::getVolume() const {
    llong square = getSquare();
    llong height = getHeight();
    llong volume = square * height;
    return volume;
}


void LandAABB::fix() {
    if (min.x > max.x) std::swap(min.x, max.x);
    if (min.y > max.y) std::swap(min.y, max.y);
    if (min.z > max.z) std::swap(min.z, max.z);
}
LandAABB LandAABB::make(BlockPos const& min, BlockPos const& max) {
    LandAABB inst(LandPos::make(min), LandPos::make(max));
    inst.fix();
    return inst;
}
std::string LandAABB::toString() const { return fmt::format("{} => {}", min.toString(), max.toString()); }
std::unordered_set<ChunkPos> LandAABB::getChunks() const {
    std::unordered_set<ChunkPos> chunks;

    int minChunkX = min.x >> 4;
    int minChunkZ = min.z >> 4;
    int maxChunkX = max.x >> 4;
    int maxChunkZ = max.z >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            chunks.insert(ChunkPos{x, z});
        }
    }
    return chunks;
}

std::vector<BlockPos> LandAABB::getBorder() const {
    std::vector<BlockPos> border;
    for (int x = min.x; x <= max.x; ++x) {
        border.emplace_back(x, min.y, min.z);
        border.emplace_back(x, min.y, max.z);
        border.emplace_back(x, max.y, min.z);
        border.emplace_back(x, max.y, max.z);
    }
    for (int y = min.y + 1; y < max.y; ++y) {
        border.emplace_back(min.x, y, min.z);
        border.emplace_back(min.x, y, max.z);
        border.emplace_back(max.x, y, min.z);
        border.emplace_back(max.x, y, max.z);
    }
    for (int z = min.z + 1; z < max.z; ++z) {
        border.emplace_back(min.x, min.y, z);
        border.emplace_back(min.x, max.y, z);
        border.emplace_back(max.x, min.y, z);
        border.emplace_back(max.x, max.y, z);
    }
    for (int y = min.y; y <= max.y; ++y) {
        border.emplace_back(min.x, y, min.z);
        border.emplace_back(min.x, y, max.z);
        border.emplace_back(max.x, y, min.z);
        border.emplace_back(max.x, y, max.z);
    }
    return border;
}

std::vector<BlockPos> LandAABB::getRange() const {
    std::vector<BlockPos> range;
    for (int x = min.x; x <= max.x; ++x) {
        range.emplace_back(x, max.y, min.z);
        range.emplace_back(x, max.y, max.z);
    }
    for (int z = min.z + 1; z < max.z; ++z) {
        range.emplace_back(min.x, max.y, z);
        range.emplace_back(max.x, max.y, z);
    }
    return range;
}

bool LandAABB::hasPos(const BlockPos& pos, bool ignoreY) const {
    if (ignoreY) {
        return pos.x >= min.x && pos.x <= max.x && pos.z >= min.z && pos.z <= max.z;
    } else {
        return pos.x >= min.x && pos.x <= max.x && pos.y >= min.y && pos.y <= max.y && pos.z >= min.z && pos.z <= max.z;
    }
}

LandAABB LandAABB::expanded(int spacing, bool ignoreY) const {
    return ignoreY ?
        LandAABB{
            LandPos{min.x - spacing, min.y, min.z - spacing},
            LandPos{max.x + spacing, max.y, max.z + spacing},
        } :
        LandAABB{
            LandPos{min.x - spacing, min.y - spacing, min.z - spacing},
            LandPos{max.x + spacing, max.y + spacing, max.z + spacing},
        };
}

bool LandAABB::operator==(LandAABB const& pos) const { return min == pos.min && max == pos.max; }

bool LandAABB::isCollision(const LandAABB& pos1, const LandAABB& pos2) {
    return !(
        pos1.max.x < pos2.min.x || pos1.min.x > pos2.max.x || pos1.max.y < pos2.min.y || pos1.min.y > pos2.max.y
        || pos1.max.z < pos2.min.z || pos1.min.z > pos2.max.z
    );
}
bool LandAABB::isComplisWithMinSpacing(const LandAABB& pos1, const LandAABB& pos2, int minSpacing) {
    // 检查 X 轴
    int xDist = std::min(std::abs(pos1.max.x - pos2.min.x), std::abs(pos2.max.x - pos1.min.x));
    if (xDist < minSpacing) return false;

    // 检查 Z 轴
    int zDist = std::min(std::abs(pos1.max.z - pos2.min.z), std::abs(pos2.max.z - pos1.min.z));
    if (zDist < minSpacing) return false;

    // 检查 Y 轴
    int yDist = std::min(std::abs(pos1.max.y - pos2.min.y), std::abs(pos2.max.y - pos1.min.y));
    if (yDist < minSpacing) return false;

    return true;
}
bool LandAABB::isContain(LandAABB const& src, LandAABB const& dst) {
    return src.min.x <= dst.min.x && src.max.x >= dst.max.x && //
           src.min.y <= dst.min.y && src.max.y >= dst.max.y && //
           src.min.z <= dst.min.z && src.max.z >= dst.max.z;
}

bool LandAABB::isOnInnerBoundary(BlockPos const& pos) const {
    bool isXBoundary =
        (pos.x == min.x || pos.x == max.x) && (pos.y >= min.y && pos.y <= max.y) && (pos.z >= min.z && pos.z <= max.z);

    bool isYBoundary =
        (pos.y == min.y || pos.y == max.y) && (pos.x >= min.x && pos.x <= max.x) && (pos.z >= min.z && pos.z <= max.z);

    bool isZBoundary =
        (pos.z == min.z || pos.z == max.z) && (pos.x >= min.x && pos.x <= max.x) && (pos.y >= min.y && pos.y <= max.y);

    return isXBoundary || isYBoundary || isZBoundary;
}

bool LandAABB::isOnOuterBoundary(BlockPos const& pos) const {
    // 检查是否在领地外紧贴边界
    bool isXBoundary = (pos.x == min.x - 1 || pos.x == max.x + 1) && (pos.y >= min.y && pos.y <= max.y)
                    && (pos.z >= min.z && pos.z <= max.z);

    bool isYBoundary = (pos.y == min.y - 1 || pos.y == max.y + 1) && (pos.x >= min.x && pos.x <= max.x)
                    && (pos.z >= min.z && pos.z <= max.z);

    bool isZBoundary = (pos.z == min.z - 1 || pos.z == max.z + 1) && (pos.x >= min.x && pos.x <= max.x)
                    && (pos.y >= min.y && pos.y <= max.y);

    return isXBoundary || isYBoundary || isZBoundary;
}

bool LandAABB::isAboveLand(BlockPos const& pos) const {
    return pos.x >= min.x && pos.x <= max.x && //
           pos.z >= min.z && pos.z <= max.z && //
           pos.y > max.y;
}


int LandAABB::getMinSpacing(LandAABB const& a, LandAABB const& b) {
    // 检查是否有重叠
    if (a.max.x >= b.min.x && a.min.x <= b.max.x &&
        a.max.z >= b.min.z && a.min.z <= b.max.z) {
        // 如果有重叠，计算最大重叠深度（返回负值）
        int xOverlap = std::min(a.max.x - b.min.x, b.max.x - a.min.x);
        int zOverlap = std::min(a.max.z - b.min.z, b.max.z - a.min.z);
        return -std::min(xOverlap, zOverlap); // 返回负的重叠深度
    }

    // 如果没有重叠，计算最小间距
    int xSpacing = std::max(a.min.x - b.max.x, b.min.x - a.max.x);
    int zSpacing = std::max(a.min.z - b.max.z, b.min.z - a.max.z);
    
    // 返回最小间距（正值）
    return std::max(xSpacing, zSpacing);
}


} // namespace land