#pragma once
#include "mc/world/level/dimension/DimensionType.h"
#include "pland/drawer/IDrawerHandle.h"
#include "pland/Global.h"

namespace land::drawer::detail {

class DebugShapeHandle final : public IDrawerHandle {
    struct Impl;
    std::unique_ptr<Impl> impl_;

public:
    DebugShapeHandle();
    ~DebugShapeHandle() override;

    GeoId draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) override;

    void draw(std::shared_ptr<Land> const& land, mce::Color const& color) override;

    void remove(GeoId id) override;

    void remove(LandID landId) override;

    void remove(std::shared_ptr<Land> land) override;

    void clear() override;

    void clearLand() override;

    static bool isDebugShapeLoaded();
};

} // namespace land::drawer::detail
