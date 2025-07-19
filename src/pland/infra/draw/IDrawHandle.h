#pragma once
#include "mc/deps/core/math/Color.h"
#include "pland/Global.h"

#include <mc/deps/core/utility/AutomaticID.h>


namespace land {


class LandAABB;
class Land;


struct GeoId {
    uint64 value{0};

    operator bool() const { return value != 0; }
    bool operator==(GeoId const& other) const { return value == other.value; }

    GeoId() = default;
    explicit GeoId(uint64 v) : value(v) {}
};

class IDrawHandle {
public:
    LD_DISALLOW_COPY_AND_MOVE(IDrawHandle);

    LDAPI explicit IDrawHandle() = default;
    LDAPI virtual ~IDrawHandle() = default;

    virtual GeoId draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) = 0;

    virtual void draw(std::shared_ptr<Land> const& land, mce::Color const& color) = 0;

    virtual void remove(GeoId id) = 0;

    virtual void remove(LandID landId) = 0;

    virtual void remove(std::shared_ptr<Land> land) = 0;

    virtual void clear() = 0;

    virtual void clearLand() = 0;
};


} // namespace land


// Fix std::unordered_map<land::GeoId, ...>
namespace std {
template <>
struct hash<land::GeoId> {
    size_t operator()(land::GeoId const& id) const { return std::hash<uint64>()(id.value); }
};
} // namespace std