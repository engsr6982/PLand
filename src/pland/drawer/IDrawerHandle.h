#pragma once
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/utility/optional_ref.h"
#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "pland/Global.h"


class Player;

namespace land {
class LandAABB;
class Land;
} // namespace land

namespace land::drawer {


struct GeoId {
    uint64 value{0};

    constexpr      operator bool() const { return value != 0; }
    constexpr bool operator==(GeoId const& other) const { return value == other.value; }

    GeoId() = default;
    explicit GeoId(uint64 v) : value(v) {}
};

class IDrawerHandle {
protected:
    WeakEntityRef mTargetPlayer;

public:
    LD_DISABLE_COPY_AND_MOVE(IDrawerHandle);

    LDAPI IDrawerHandle();
    LDAPI virtual ~IDrawerHandle();

    LDAPI void setTargetPlayer(Player& player);

    LDNDAPI optional_ref<Player> getTargetPlayer() const;

    virtual GeoId draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) = 0;

    virtual void draw(std::shared_ptr<Land> const& land, mce::Color const& color) = 0;

    virtual void remove(GeoId id) = 0;

    virtual void remove(LandID landId) = 0;

    virtual void remove(std::shared_ptr<Land> land) = 0;

    virtual void clear() = 0;

    virtual void clearLand() = 0;
};


} // namespace land::drawer


// Fix std::unordered_map<land::GeoId, ...>
namespace std {
template <>
struct hash<land::drawer::GeoId> {
    constexpr size_t operator()(land::drawer::GeoId const& id) const noexcept { return id.value; }
};
} // namespace std