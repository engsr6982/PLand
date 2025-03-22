#pragma once
#include "ll/api/service/Bedrock.h"
#include "mc/world/level/Level.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include "pland/wrapper/FormEx.h"


namespace land {

class ChooseLandUtilGui {
public:
    using ChooseCallback = std::function<void(Player&, LandID id)>;
    template <typename ParentForm = void>
    static void impl(Player& player, ChooseCallback const& callback, bool showShredLand = false) {
        auto fm = SimpleFormEx::create<ParentForm, BackButtonPos::Upper>();
        fm.setTitle(PLUGIN_NAME + ("| 选择领地"_trf(player)));
        fm.setContent("请选择一个领地"_trf(player));

        auto lands = PLand::getInstance().getLands(player.getUuid().asString(), showShredLand);
        for (auto& land : lands) {
            fm.appendButton(
                "{}\n维度: {} | ID: {}"_trf(player, land->getLandName(), land->getLandDimid(), land->getLandID()),
                "textures/ui/icon_recipe_nature",
                [callback, land](Player& pl) { callback(pl, land->getLandID()); }
            );
        }

        fm.sendTo(player);
    }
};


class ChoosePlayerUtilGui {
public:
    using ChoosePlayerCall = std::function<void(Player&, Player& choosedPlayer)>;
    template <typename ParentForm = void>
    static void impl(Player& player, ChoosePlayerCall const& callback) {
        auto fm = SimpleFormEx::create<LandMainGui, BackButtonPos::Upper>();
        fm.setTitle(PLUGIN_NAME + ("| 选择玩家"_trf(player)));

        ll::service::getLevel()->forEachPlayer([callback, &fm](Player& target) {
            if (target.isSimulatedPlayer()) {
                return true; // ignore
            }

            fm.appendButton(target.getRealName(), "", [callback, &target](Player& self) { callback(self, target); });
            return true;
        });

        fm.sendTo(player);
    }
};


class EditStringUtilGui {
public:
    using EditStringResult = std::function<void(Player& self, string result)>;
    LDAPI static void impl(
        Player&                 player,
        string const&           title,       // 标题
        string const&           text,        // 提示
        string const&           defaultValu, // 默认值
        EditStringResult const& callback     // 回调
    );
};


class FuzzySerarchUtilGui {
public:
    using CallBack = std::function<void(Player& slef, std::vector<LandData_sptr> const& result)>;
    LDAPI static void impl(Player& player, std::vector<LandData_sptr> const& list, CallBack const& callback);
};


} // namespace land