#pragma once
#include "pland/infra/draw/GeoId.h"
#include "pland/infra/draw/IDrawHandle.h"

class AABB;

namespace land {

class Land;
class LandPos;

class BsciDrawHandle final : public IDrawHandle {
    class Impl;
    std::unique_ptr<Impl> impl;

public:
    LDAPI explicit BsciDrawHandle();
    LDAPI ~BsciDrawHandle() override;

    LDNDAPI GeoIdPtr draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) override;

    LDAPI void draw(std::shared_ptr<Land> const& land, mce::Color const& color) override;

    LDAPI void remove(GeoIdPtr id) override;

    LDAPI void remove(std::shared_ptr<Land> land) override;

    LDAPI void clear() override;

    LDAPI void clearLand() override;

    LDNDAPI AABB fixAABB(LandPos const& min, LandPos const& max);
    LDNDAPI AABB fixAABB(LandAABB const& aabb);

    LDAPI static bool isBsciModuleLoaded();
};


} // namespace land