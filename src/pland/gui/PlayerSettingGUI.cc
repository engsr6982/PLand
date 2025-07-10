#include "PlayerSettingGUI.h"
#include "ll/api/form/CustomForm.h"
#include "mc/world/actor/player/Player.h"
#include "pland/land/LandRegistry.h"
#include "pland/utils/McUtils.h"


namespace land {


void PlayerSettingGUI::sendTo(Player& player) {
    using namespace ll::form;

    auto setting = LandRegistry::getInstance().getPlayerSettings(player.getUuid().asString());

    CustomForm fm(PLUGIN_NAME + ("| 玩家设置"_trf(player)));

    fm.appendToggle("showEnterLandTitle", "是否显示进入领地提示"_trf(player), setting->showEnterLandTitle);
    fm.appendToggle("showBottomContinuedTip", "是否持续显示底部提示"_trf(player), setting->showBottomContinuedTip);

    fm.sendTo(player, [setting](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res) {
            return;
        }

        setting->showEnterLandTitle     = std::get<uint64_t>(res->at("showEnterLandTitle"));
        setting->showBottomContinuedTip = std::get<uint64_t>(res->at("showBottomContinuedTip"));

        mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "设置已保存"_trf(pl));
    });
}


} // namespace land