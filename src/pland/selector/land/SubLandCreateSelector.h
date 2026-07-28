#pragma once
#include "pland/selector/ABSelector.h"


namespace land {

class Land;

class SubLandCreateSelector final : public ABSelector {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    LDAPI explicit SubLandCreateSelector(Player& player, std::shared_ptr<Land> parent);
    LDAPI ~SubLandCreateSelector() override;

    LDNDAPI std::shared_ptr<Land> getParentLand() const;

    LDNDAPI std::shared_ptr<Land> newSubLand() const;
};


} // namespace land