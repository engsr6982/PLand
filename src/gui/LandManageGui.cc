#include "pland/gui/LandManageGui.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Config.h"
#include "pland/EconomySystem.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandEvent.h"
#include "pland/LandPos.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/PriceCalculate.h"
#include "pland/gui/CommonUtilGui.hpp"
#include "pland/gui/LandManageGui.h"
#include "pland/utils/JSON.h"
#include "pland/utils/McUtils.h"
#include "pland/wrapper/FormEx.h"
#include <cstdint>
#include <string>


using namespace ll::form;

namespace land {

void LandManageGui::impl(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地不存在"_trf(player));
        return;
    }

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 领地管理 [{}]"_trf(player, land->getLandID())));
    fm.setContent("领地: {}\n类型: {}\n大小: {}x{}x{} = {}\n范围: {}\n"_trf( 
        player,
        land->getLandName(),
        land->is3DLand() ? "3D" : "2D",
        land->getLandPos().getDepth(),
        land->getLandPos().getWidth(),
        land->getLandPos().getHeight(),
        land->getLandPos().getVolume(),
        land->getLandPos().toString()
    ));

    fm.appendButton("编辑权限"_trf(player), "textures/ui/sidebar_icons/promotag", [land](Player& pl) {
        EditLandPermGui::impl(pl, land);
    });
    fm.appendButton("修改成员"_trf(player), "textures/ui/FriendsIcon", [land](Player& pl) {
        EditLandMemberGui::impl(pl, land);
    });
    fm.appendButton("修改领地名称"_trf(player), "textures/ui/book_edit_default", [land](Player& pl) {
        EditLandNameGui::impl(pl, land);
    });
    fm.appendButton("修改领地描述"_trf(player), "textures/ui/book_edit_default", [land](Player& pl) {
        EditLandDescGui::impl(pl, land);
    });
    fm.appendButton("传送到领地", "textures/ui/icon_recipe_nature", [id](Player& pl) { LandTeleportGui::run(pl, id); });
    fm.appendButton("领地过户"_trf(player), "textures/ui/sidebar_icons/my_characters", [land](Player& pl) {
        EditLandOwnerGui::impl(pl, land);
    });
    fm.appendButton("重新选区"_trf(player), "textures/ui/anvil_icon", [land](Player& pl) {
        ReSelectLandGui::impl(pl, land);
    });
    fm.appendButton("删除领地"_trf(player), "textures/ui/icon_trash", [land](Player& pl) {
        DeleteLandGui::impl(pl, land);
    });

    fm.sendTo(player);
}
void LandManageGui::EditLandPermGui::impl(Player& player, LandData_sptr const& ptr) {
    CustomForm fm(PLUGIN_NAME + " | 编辑权限"_trf(player));

    auto& i18n = ll::i18n::getInstance();

    auto localeCode = GetPlayerLocaleCodeFromSettings(player);

    auto js = JSON::structTojson(ptr->getLandPermTableConst());
    for (auto& [k, v] : js.items()) {
        fm.appendToggle(k, (string)i18n.get(k, localeCode), v);
    }

    fm.sendTo(player, [ptr](Player& pl, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        auto& perm = ptr->getLandPermTable();
        auto  j    = JSON::structTojson(perm);
        for (auto const& [key, value] : j.items()) {
            bool const val = std::get<uint64_t>(res->at(key));
            j[key]         = val;
        }

        JSON::jsonToStructNoMerge(j, perm);

        mc_utils::sendText(pl, "权限表已更新"_trf(pl));
    });
}
void LandManageGui::DeleteLandGui::impl(Player& player, LandData_sptr const& ptr) {
    int price = PriceCalculate::calculateRefundsPrice(ptr->mOriginalBuyPrice, Config::cfg.land.refundRate);

    PlayerDeleteLandBeforeEvent ev(player, ptr->getLandID(), price);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    ModalForm fm(
        PLUGIN_NAME + " | 确认删除?"_trf(player),
        "您确定要删除领地 {} 吗?\n删除领地后，您将获得 {} 金币的退款。\n此操作不可逆,请谨慎操作!"_trf(
            player,
            ptr->getLandName(),
            price
        ),
        "确认"_trf(player),
        "返回"_trf(player)
    );
    fm.sendTo(player, [ptr, price](Player& pl, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        if (!(bool)res.value()) {
            LandManageGui::impl(pl, ptr->getLandID());
            return;
        }

        auto& db  = PLand::getInstance();
        auto& eco = EconomySystem::getInstance();

        if (eco.add(pl, price)) {
            if (db.removeLand(ptr->getLandID())) {
                mc_utils::sendText(pl, "删除领地成功!"_trf(pl));
            } else mc_utils::sendText(pl, "删除领地失败!"_trf(pl));

            PlayerDeleteLandAfterEvent ev(pl, ptr->getLandID());
            ll::event::EventBus::getInstance().publish(ev);

        } else mc_utils::sendText(pl, "经济系统异常，操作失败"_trf(pl));
    });
}
void LandManageGui::EditLandNameGui::impl(Player& player, LandData_sptr const& ptr) {
    EditStringUtilGui::impl(
        player,
        "修改领地名称"_trf(player),
        "请输入新的领地名称"_trf(player),
        ptr->getLandName(),
        [ptr](Player& pl, string result) {
            ptr->setLandName(result);
            mc_utils::sendText(pl, "领地名称已更新!"_trf(pl));
        }
    );
}
void LandManageGui::EditLandDescGui::impl(Player& player, LandData_sptr const& ptr) {
    EditStringUtilGui::impl(
        player,
        "修改领地描述"_trf(player),
        "请输入新的领地描述"_trf(player),
        ptr->getLandDescribe(),
        [ptr](Player& pl, string result) {
            ptr->setLandDescribe(result);
            mc_utils::sendText(pl, "领地描述已更新!"_trf(pl));
        }
    );
}
void LandManageGui::EditLandOwnerGui::impl(Player& player, LandData_sptr const& ptr) {
    ChoosePlayerUtilGui::impl(player, [ptr](Player& self, Player& target) {
        if (self.getUuid() == target.getUuid()) {
            mc_utils::sendText(self, "不能将领地转让给自己, 左手倒右手哦!"_trf(self));
            return;
        }

        LandOwnerChangeBeforeEvent ev(self, target, ptr->getLandID());
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm fm(
            PLUGIN_NAME + " | 确认转让?"_trf(self),
            "您确定要将领地转让给 {} 吗?\n转让后，您将失去此领地的权限。\n此操作不可逆,请谨慎操作!"_trf(
                self,
                target.getRealName()
            ),
            "确认"_trf(self),
            "返回"_trf(self)
        );
        fm.sendTo(self, [ptr, &target](Player& self, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }

            if (!(bool)res.value()) {
                LandManageGui::impl(self, ptr->getLandID());
                return;
            }

            if (ptr->setLandOwner(target.getUuid().asString())) {
                mc_utils::sendText(self, "领地已转让给 {}"_trf(self, target.getRealName()));
                mc_utils::sendText(
                    target,
                    "您已成功接手来自 \"{}\" 的领地 \"{}\""_trf(self, self.getRealName(), ptr->getLandName())
                );

                LandOwnerChangeAfterEvent ev(self, target, ptr->getLandID());
                ll::event::EventBus::getInstance().publish(ev);
            } else {
                mc_utils::sendText<mc_utils::LogLevel::Error>(self, "领地转让失败!"_trf(self));
            }
        });
    });
}
void LandManageGui::ReSelectLandGui::impl(Player& player, LandData_sptr const& ptr) {
    ModalForm fm(
        PLUGIN_NAME + " | 重新选区"_trf(player),
        "重新选区为完全重新选择领地的范围，非直接扩充/缩小现有领地范围。\n重新选择的价格计算方式为\"新范围价格 — 旧范围价值\"，是否继续？"_trf(
            player
        ),
        "确认"_trf(player),
        "返回"_trf(player)
    );
    fm.sendTo(player, [ptr](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        if (!(bool)res.value()) {
            LandManageGui::impl(self, ptr->getLandID());
            return;
        }

        auto selector = std::make_unique<LandReSelector>(self, ptr);
        if (!SelectorManager::getInstance().start(std::move(selector))) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "选区开启失败，当前存在未完成的选区任务"_trf(self));
        }
    });
}

} // namespace land