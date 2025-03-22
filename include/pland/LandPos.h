#pragma once
#include "Global.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"


namespace land {


// 坐标基类
class PosBase {
public:
    int x, y, z;

    LDNDAPI static PosBase make(int x, int y, int z);
    LDNDAPI static PosBase make(BlockPos const& pos);

    LDNDAPI std::string toString() const;

    LDNDAPI bool isZero() const; // xyz是否都为0

    LDNDAPI int distance(Vec3 const& pos) const; // 获取到pos的距离

    LDAPI PosBase& operator=(PosBase const& pos) = default;
    LDAPI bool     operator==(PosBase const& pos) const;
    LDAPI bool     operator!=(PosBase const& pos) const;
    LDAPI PosBase& operator=(BlockPos const& pos);
    LDAPI bool     operator==(BlockPos const& pos) const;
    LDAPI bool     operator!=(BlockPos const& pos) const;
    LDAPI          operator BlockPos() const;
    LDAPI          operator Vec3() const;
};


// 领地坐标
class LandPos {
public:
    PosBase mMin_A, mMax_B;

    LDNDAPI static LandPos make(BlockPos const& min, BlockPos const& max);

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

    LDAPI bool operator==(LandPos const& pos) const;
    LDAPI bool operator!=(LandPos const& pos) const;

    /**
     * @brief 判断两个 AABB 是否有重叠部分
     */
    LDNDAPI static bool isCollision(LandPos const& pos1, LandPos const& pos2);

    /**
     * @brief 判断两个AABB是否满足最小间距要求
     */
    LDNDAPI static bool isComplisWithMinSpacing(LandPos const& pos1, LandPos const& pos2, int minSpacing);

    /**
     * @brief 判断一个 AABB 区域是否完整包含另一个 AABB 区域
     * 如果目标 AABB 在源 AABB 内，则返回 true，否则返回 false
     */
    LDNDAPI static bool isContain(LandPos const& src, LandPos const& dst);
};


} // namespace land