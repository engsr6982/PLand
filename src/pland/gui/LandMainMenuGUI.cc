#include "LandMainMenuGUI.h"
#include "LandManagerGUI.h"
#include "NewLandGUI.h"
#include "PlayerSettingGUI.h"
#include "pland/PLand.h"
#include "pland/gui/CommonUtilGui.h"
#include "pland/gui/LandManagerGUI.h"
#include "pland/gui/LandTeleportGUI.h"
#include "pland/gui/NewLandGUI.h"
#include "pland/gui/common/ChooseLandAdvancedUtilGUI.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"


namespace land {


void LandMainMenuGUI::sendTo(Player& player) {
    auto fm = BackSimpleForm<>::make();
    fm.setTitle(PLUGIN_NAME + ("| 领地菜单"_trf(player)));
    fm.setContent("欢迎使用 Pland 领地管理插件\n\n请选择一个功能"_trf(player));

    fm.appendButton("新建领地"_trf(player), "textures/ui/anvil_icon", "path", [](Player& pl) {
        NewLandGUI::sendChooseLandDim(pl);
    });

    fm.appendButton("管理领地"_trf(player), "textures/ui/icon_spring", "path", [](Player& pl) {
        // ChooseLandUtilGUI::sendTo(pl, LandManagerGUI::sendMainMenu, false, BackSimpleForm<>::makeCallback<sendTo>());
        ChooseLandAdvancedUtilGUI::sendTo(
            pl,
            PLand::getInstance().getLandRegistry()->getLands(pl.getUuid().asString()),
            LandManagerGUI::sendMainMenu,
            BackSimpleForm<>::makeCallback<sendTo>()
        );
    });

    if (Config::cfg.land.landTp || PLand::getInstance().getLandRegistry()->isOperator(player.getUuid().asString())) {
        fm.appendButton("领地传送"_trf(player), "textures/ui/icon_recipe_nature", "path", [](Player& pl) {
            LandTeleportGUI::sendTo(pl);
        });
    }

    fm.appendButton("个人设置"_trf(player), "textures/ui/icon_setting", "path", [](Player& pl) {
        PlayerSettingGUI::sendTo(pl);
    });

    fm.appendButton("关闭"_trf(player), "textures/ui/cancel", "path");
    fm.sendTo(player);
}


} // namespace land