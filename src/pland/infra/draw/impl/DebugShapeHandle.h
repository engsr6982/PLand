#pragma once
#include "pland/Global.h"
#include "pland/infra/draw/IDrawHandle.h"

namespace land {

class DebugShapeHandle final : public IDrawHandle {
    struct Impl;
    std::unique_ptr<Impl> impl_;

public:
    LDAPI DebugShapeHandle();
    LDAPI ~DebugShapeHandle() override;

    LDNDAPI GeoId draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) override;

    LDAPI void draw(std::shared_ptr<Land> const& land, mce::Color const& color) override;

    LDAPI void remove(GeoId id) override;

    LDAPI void remove(LandID landId) override;

    LDAPI void remove(std::shared_ptr<Land> land) override;

    LDAPI void clear() override;

    LDAPI void clearLand() override;
};

} // namespace land
