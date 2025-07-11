#include "LandTeleportGUI.h"
#include "CommonUtilGUI.h"
#include "pland/gui/LandMainMenuGUI.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/infra/SafeTeleport.h"
#include "pland/land/Land.h"
#include "pland/land/LandRegistry.h"
#include "pland/utils/McUtils.h"


namespace land {


void LandTeleportGUI::sendTo(Player& player) {
    ChooseLandUtilGUI::sendTo(player, impl, true, BackSimpleForm<>::makeCallback<LandMainMenuGUI::sendTo>());
}

void LandTeleportGUI::impl(Player& player, SharedLand land) {
    if (land->getTeleportPos().isZero()) {
        SafeTeleport::getInstance().teleportTo(player, land->getAABB().getMin().as(), land->getDimensionId());
        return;
    }
    player.teleport(land->getTeleportPos().as(), land->getDimensionId());
}


} // namespace land