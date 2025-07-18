#include "pland/gui/CommonUtilGUI.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/service/Bedrock.h"
#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/land/Land.h"
#include "pland/land/LandRegistry.h"
#include <algorithm>
#include <string>


namespace land {
using namespace ll::form;


void ChooseLandUtilGUI::sendTo(
    Player&                          player,
    ChooseCallback const&            callback,
    bool                             showShredLand,
    BackSimpleForm<>::ButtonCallback back
) {
    auto fm = BackSimpleForm<>{std::move(back)};
    fm.setTitle(PLUGIN_NAME + ("| 选择领地"_trf(player)));
    fm.setContent("请选择一个领地"_trf(player));

    auto lands = PLand::getInstance().getLandRegistry()->getLands(player.getUuid().asString(), showShredLand);
    for (auto& land : lands) {
        fm.appendButton(
            "{}\n维度: {} | ID: {}"_trf(player, land->getName(), land->getDimensionId(), land->getId()),
            "textures/ui/icon_recipe_nature",
            "path",
            [callback, land = std::weak_ptr(land)](Player& pl) {
                if (auto p = land.lock()) {
                    callback(pl, p);
                }
            }
        );
    }

    fm.sendTo(player);
}

void ChooseOnlinePlayerUtilGUI::sendTo(
    Player&                          player,
    ChoosePlayerCall const&          callback,
    BackSimpleForm<>::ButtonCallback back
) {
    auto fm = BackSimpleForm<>{std::move(back)};
    fm.setTitle(PLUGIN_NAME + ("| 选择玩家"_trf(player)));

    ll::service::getLevel()->forEachPlayer([callback, &fm](Player& target) {
        if (target.isSimulatedPlayer()) {
            return true; // ignore
        }

        fm.appendButton(target.getRealName(), [callback, weakRef = target.getWeakEntity()](Player& self) {
            if (auto target = weakRef.tryUnwrap<Player>()) {
                callback(self, target);
            }
        });
        return true;
    });

    fm.sendTo(player);
}


void EditStringUtilGUI::sendTo(
    Player&          player,
    string const&    title,        // 标题
    string const&    text,         // 提示
    string const&    defaultValue, // 默认值
    EditStringResult callback      // 回调
) {
    CustomForm fm(PLUGIN_NAME + title);
    fm.appendInput("str", text, "string", defaultValue);
    fm.sendTo(player, [cb = std::move(callback)](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }
        cb(pl, std::get<string>(res->at("str")));
    });
}


void FuzzySerarchUtilGUI::sendTo(Player& player, std::vector<SharedLand> list, CallBack callback) {
    CustomForm fm;
    fm.setTitle(PLUGIN_NAME + " | 模糊搜索领地"_trf(player));
    fm.appendInput("name", "请输入领地名称"_trf(player), "string");
    fm.sendTo(
        player,
        [list = std::move(list),
         cb   = std::move(callback)](Player& player, CustomFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }
            auto                    name = std::get<string>(res->at("name"));
            std::vector<SharedLand> filtered;
            for (auto const& ptr : list) {
                if (ptr->getName().find(name) != std::string::npos) {
                    filtered.push_back(ptr);
                }
            }
            cb(player, std::move(filtered));
        }
    );
}


} // namespace land