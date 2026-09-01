#include "OrdinaryLandCreateSelector.h"

#include "pland/land/Land.h"
#include "pland/selector/ABSelector.h"
#include "pland/selector/land/OrdinaryLandCreateSelector.h"

#include "mc/world/actor/player/Player.h"

namespace land {


OrdinaryLandCreateSelector::OrdinaryLandCreateSelector(Player& player, bool is3D)
: ABSelector(player, player.getDimensionId(), is3D) {}

std::shared_ptr<Land> OrdinaryLandCreateSelector::newLand() const {
    if (!isPointABSet()) {
        return nullptr;
    }

    auto player = getPlayer();
    if (!player) {
        return nullptr;
    }

    auto land = Land::make(*newLandAABB(), getDimensionId(), is3D(), player->getUuid());
    land->markDirty();
    return land;
}


} // namespace land