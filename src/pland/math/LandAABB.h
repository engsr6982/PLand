#pragma once
#include "pland/Global.h"
#include "pland/math/LandPos.h"

namespace land {


/**
 * @brief 领地坐标范围
 */
class LandAABB {
public:
    LandPos min, max;

    LDNDAPI static LandAABB make(BlockPos const& min, BlockPos const& max);

    LDAPI void fix(); // fix min/max

    LDNDAPI int getDepth() const;  // (长) X
    LDNDAPI int getHeight() const; // (高) Y
    LDNDAPI int getWidth() const;  // (宽) Z
    LDNDAPI int getSquare() const; // (底面积) X * Z
    LDNDAPI int getVolume() const; // (总体积) Z * X * Y

    LDNDAPI std::string toString() const;

    LDNDAPI std::vector<ChunkPos> getChunks() const;
    LDNDAPI std::vector<BlockPos> getBorder() const;
    LDNDAPI std::vector<BlockPos> getRange() const;

    LDNDAPI bool hasPos(BlockPos const& pos, bool ignoreY = false) const;

    // 判断某个pos是否在领地内边界
    LDNDAPI bool isOnInnerBoundary(BlockPos const& pos) const;
    // 判断某个pos是否在领地外边界
    LDNDAPI bool isOnOuterBoundary(BlockPos const& pos) const;
    // 检查位置是否在领地上方（x/z 在领地范围内，且 y > max.y）
    LDNDAPI bool isAboveLand(BlockPos const& pos) const;

    LDAPI bool operator==(LandAABB const& pos) const;
    LDAPI bool operator!=(LandAABB const& pos) const;

    /**
     * @brief 判断两个 AABB 是否有重叠部分
     */
    LDNDAPI static bool isCollision(LandAABB const& pos1, LandAABB const& pos2);

    /**
     * @brief 判断两个AABB是否满足最小间距要求
     */
    LDNDAPI static bool isComplisWithMinSpacing(LandAABB const& pos1, LandAABB const& pos2, int minSpacing);

    /**
     * @brief 判断一个 AABB 区域是否完整包含另一个 AABB 区域
     * 如果目标 AABB 在源 AABB 内，则返回 true，否则返回 false
     */
    LDNDAPI static bool isContain(LandAABB const& src, LandAABB const& dst);
};


} // namespace land