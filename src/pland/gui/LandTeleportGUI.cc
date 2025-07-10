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

void LandTeleportGUI::impl(Player& player, Land_sptr land) {
    if (land->mTeleportPos.isZero()) {
        SafeTeleport::getInstance().teleportTo(player, land->mPos.min, land->getLandDimid());
        return;
    }

    player.teleport(land->mTeleportPos, land->getLandDimid());
}


} // namespace land