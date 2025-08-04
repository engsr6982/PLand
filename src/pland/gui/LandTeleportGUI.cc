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
        PLand::getInstance().getLandRegistry()->getLands(player.getUuid().asString(), true),
        impl,
        BackSimpleForm<>::makeCallback<sendTo>()
    );
}

void LandTeleportGUI::impl(Player& player, SharedLand land) {
    auto const& tpPos = land->getTeleportPos();
    if (tpPos.isZero() || !land->getAABB().hasPos(tpPos.as<Vec3>())) {
        if (!tpPos.isZero()) {
            land->setTeleportPos(LandPos::make(0, 0, 0));
        }
        PLand::getInstance().getSafeTeleport()->launchTask(
            player,
            {land->getAABB().getMin().as(), land->getDimensionId()}
        );
        return;
    }
    auto v3 = tpPos.as<Vec3>();
    player.teleport(v3, land->getDimensionId());
}


} // namespace land