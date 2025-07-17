#pragma once
#include "pland/selector/ISelector.h"


namespace land {

class Land;

class SubLandSelector final : public ISelector {
    std::weak_ptr<Land> mParentLand;
    GeoId               mParentRangeDrawId;

public:
    LDAPI explicit SubLandSelector(Player& player, std::shared_ptr<Land> parent);
    LDAPI ~SubLandSelector() override;

    LDNDAPI std::shared_ptr<Land> getParentLand() const;

    LDNDAPI std::shared_ptr<Land> newSubLand() const;
};


} // namespace land