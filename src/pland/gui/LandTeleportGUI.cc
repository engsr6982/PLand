#include "LandTeleportGUI.h"
#include "CommonUtilGUI.h"
#include "pland/PLand.h"
#include "pland/gui/LandMainMenuGUI.h"
#include "pland/gui/common/ChooseLandAdvancedUtilGUI.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/infra/SafeTeleport.h"
#include "pland/land/Land.h"
#include "pland/land/LandRegistry.h"
#include "pland/utils/McUtils.h"


namespace land {


void LandTeleportGUI::sendTo(Player& player) {
    // ChooseLandUtilGUI::sendTo(player, impl, true, BackSimpleForm<>::makeCallback<LandMainMenuGUI::sendTo>());
    ChooseLandAdvancedUtilGUI::sendTo(
        player,
        LandRegistry::getInstance().getLands(player.getUuid().asString(), true),
        impl,
        BackSimpleForm<>::makeCallback<sendTo>()
    );
}

void LandTeleportGUI::impl(Player& player, SharedLand land) {
    if (land->getTeleportPos().isZero()) {
        PLand::getInstance().getSafeTeleport()->launchTask(
            player,
            {land->getAABB().getMin().as(), land->getDimensionId()}
        );
        return;
    }
    player.teleport(land->getTeleportPos().as(), land->getDimensionId());
}


} // namespace land