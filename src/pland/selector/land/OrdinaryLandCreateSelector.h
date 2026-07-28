#pragma once
#include "pland/selector/ABSelector.h"


namespace land {

class Land;

class OrdinaryLandCreateSelector final : public ABSelector {
public:
    LDAPI explicit OrdinaryLandCreateSelector(Player& player, bool is3D);

    LDNDAPI std::shared_ptr<Land> newLand() const;
};


} // namespace land