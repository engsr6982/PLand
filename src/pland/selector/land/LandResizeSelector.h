#pragma once
#include "pland/selector/ABSelector.h"

#include <memory>


namespace land {

class Land;

class LandResizeSelector final : public ABSelector {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    LDAPI explicit LandResizeSelector(Player& player, std::shared_ptr<Land> land);
    LDAPI ~LandResizeSelector() override;

    LDNDAPI std::shared_ptr<Land> getLand() const;
};


} // namespace land