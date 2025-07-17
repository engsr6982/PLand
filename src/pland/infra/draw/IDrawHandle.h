#pragma once
#include "mc/deps/core/math/Color.h"
#include "pland/Global.h"
#include "pland/infra/draw/GeoId.h"

#include <mc/deps/core/utility/AutomaticID.h>
namespace land {


class LandAABB;
class Land;

class IDrawHandle {
public:
    LD_DISALLOW_COPY_AND_MOVE(IDrawHandle);

    LDAPI explicit IDrawHandle() = default;
    LDAPI virtual ~IDrawHandle() = default;

    virtual GeoIdPtr draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) = 0;

    virtual void draw(std::shared_ptr<Land> const& land, mce::Color const& color) = 0;

    virtual void remove(GeoIdPtr id) = 0;

    virtual void remove(std::shared_ptr<Land> land) = 0;

    virtual void clear() = 0;

    virtual void clearLand() = 0;
};


} // namespace land