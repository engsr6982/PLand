#pragma once
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "pland/Global.h"


namespace land {


/**
 * @brief 领地坐标
 */
class LandPos {
public:
    int x, y, z;

    LDNDAPI static LandPos make(int x, int y, int z);
    LDNDAPI static LandPos make(BlockPos const& pos);

    LDNDAPI std::string toString() const;

    LDNDAPI bool isZero() const; // xyz是否都为0

    LDNDAPI int distance(Vec3 const& pos) const; // 获取到pos的距离

    LDAPI LandPos& operator=(LandPos const& pos) = default;
    LDAPI bool     operator==(LandPos const& pos) const;
    LDAPI bool     operator!=(LandPos const& pos) const;
    LDAPI LandPos& operator=(BlockPos const& pos);
    LDAPI bool     operator==(BlockPos const& pos) const;
    LDAPI bool     operator!=(BlockPos const& pos) const;
    LDAPI          operator BlockPos() const;
    LDAPI          operator Vec3() const;
};


} // namespace land
