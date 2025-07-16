#include "pland/aabb/LandPos.h"
#include "fmt/format.h"
#include "mc/world/level/BlockPos.h"
#include "pland/infra/Config.h"
#include <utility>


namespace land {


std::string LandPos::toString() const { return fmt::format("({},{},{})", x, y, z); }

bool LandPos::isZero() const { return x == 0 && y == 0 && z == 0; }

LandPos LandPos::make(int x, int y, int z) { return LandPos{x, y, z}; }

int LandPos::distance(Vec3 const& pos) const {
    float dx = pos.x - (float)x;
    float dy = pos.y - (float)y;
    float dz = pos.z - (float)z;
    return (int)std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace land
