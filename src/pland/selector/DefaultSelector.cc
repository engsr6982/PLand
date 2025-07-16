#pragma once
#include "DefaultSelector.h"
#include "pland/land/Land.h"
#include "pland/selector/DefaultSelector.h"
#include "pland/selector/ISelector.h"


namespace land {


DefaultSelector::DefaultSelector(Player& player, bool alwaysUseDimensionHeight)
: ISelector(player, player.getDimensionId(), alwaysUseDimensionHeight) {}

SharedLand DefaultSelector::newLand() const {
    if (!isPointABSet()) {
        return nullptr;
    }

    auto player = getPlayer();
    if (!player) {
        return nullptr;
    }

    return Land::make(*newLandAABB(), getDimensionId(), !isAlwaysUseDimensionHeight(), player->getUuid().asString());
}


} // namespace land