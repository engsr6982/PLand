#include "LandTeleportGUI.h"
#include "CommonUtilGUI.h"
#include "pland/PLand.h"
#include "pland/SafeTeleport.h"
#include "pland/gui/LandMainMenuGUI.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/utils/McUtils.h"


namespace land {


void LandTeleportGUI::sendTo(Player& player) {
    ChooseLandUtilGUI::sendTo(player, impl, true, BackSimpleForm<>::makeCallback<LandMainMenuGUI::sendTo>());
}

void LandTeleportGUI::impl(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地不存在"_trf(player));
        return;
    }

    if (land->mTeleportPos.isZero()) {
        SafeTeleport::getInstance().teleportTo(player, land->mPos.min, land->getLandDimid());
        return;
    }

    player.teleport(land->mTeleportPos, land->getLandDimid());
}


} // namespace land