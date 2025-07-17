#pragma once
#include "pland/infra/draw/IDrawHandle.h"


namespace land {


class DefaultDrawHandle final : public IDrawHandle {
    class Impl;
    std::unique_ptr<Impl> impl;

public:
    LDAPI explicit DefaultDrawHandle();
    LDAPI ~DefaultDrawHandle() override;

    LDNDAPI GeoId draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) override;

    LDAPI void draw(std::shared_ptr<Land> const& land, mce::Color const& color) override;

    LDAPI void remove(GeoId id) override;

    LDAPI void remove(LandID landId) override;

    LDAPI void remove(std::shared_ptr<Land> land) override;

    LDAPI void clear() override;

    LDAPI void clearLand() override;
};


} // namespace land