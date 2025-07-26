#pragma once
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "pland/Global.h"


namespace land {

template <typename T>
concept HasXYZ = requires(T const& t) {
    { t.x } -> std::convertible_to<int>;
    { t.y } -> std::convertible_to<int>;
    { t.z } -> std::convertible_to<int>;
};

/**
 * @brief 领地坐标
 */
class LandPos {
public:
    int x, y, z;

    LDNDAPI static LandPos make(int x, int y, int z);

    template <typename T>
        requires HasXYZ<T>
    static LandPos make(T const& t) {
        return make(static_cast<int>(t.x), static_cast<int>(t.y), static_cast<int>(t.z));
    }

    template <typename T = BlockPos>
        requires HasXYZ<T>
    T as() const {
        return {.x = x, .y = y, .z = z};
    }

    LDNDAPI std::string toString() const;

    LDNDAPI bool isZero() const; // xyz是否都为0

    LDNDAPI int distance(Vec3 const& pos) const; // 获取到pos的距离

    template <HasXYZ T>
    bool operator==(T const& t) const {
        return x == t.x && y == t.y && z == t.z;
    }

    template <HasXYZ T>
    LandPos& operator=(T const& t) {
        x = t.x;
        y = t.y;
        z = t.z;
        return *this;
    }
};


} // namespace land
