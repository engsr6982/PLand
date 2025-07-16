#pragma once
#include "pland/land/Land.h"
#include "pland/selector/ISelector.h"


namespace land {


class DefaultSelector final : public ISelector {
public:
    LDAPI explicit DefaultSelector(Player& player, bool is3D = false);

    LDNDAPI SharedLand newLand() const;
};


} // namespace land