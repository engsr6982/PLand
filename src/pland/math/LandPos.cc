#include "pland/math/LandPos.h"
#include "fmt/format.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Config.h"
#include <utility>


namespace land {


std::string LandPos::toString() const { return fmt::format("({},{},{})", x, y, z); }

bool LandPos::isZero() const { return x == 0 && y == 0 && z == 0; }

LandPos::operator BlockPos() const { return {x, y, z}; }

LandPos::operator Vec3() const { return {x, y, z}; }

bool LandPos::operator==(const LandPos& pos) const { return this->x == pos.x && this->y == pos.y && this->z == pos.z; }

bool LandPos::operator!=(const LandPos& pos) const { return !(*this == pos); }

bool LandPos::operator==(const BlockPos& pos) const { return this->x == pos.x && this->y == pos.y && this->z == pos.z; }

bool LandPos::operator!=(const BlockPos& pos) const { return !(*this == pos); }

LandPos& LandPos::operator=(const BlockPos& pos) {
    this->x = pos.x;
    this->y = pos.y;
    this->z = pos.z;
    return *this;
}

LandPos LandPos::make(int x, int y, int z) { return LandPos{x, y, z}; }

LandPos LandPos::make(BlockPos const& pos) { return LandPos{pos.x, pos.y, pos.z}; }

int LandPos::distance(Vec3 const& pos) const {
    float dx = pos.x - (float)x;
    float dy = pos.y - (float)y;
    float dz = pos.z - (float)z;
    return (int)std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace land
