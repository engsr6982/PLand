#include "pland/gui/LandManagerGUI.h"
#include "LandTeleportGUI.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/service/PlayerInfo.h"
#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Config.h"
#include "pland/DrawHandleManager.h"
#include "pland/EconomySystem.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandEvent.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/PriceCalculate.h"
#include "pland/gui/CommonUtilGUI.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/math/LandAABB.h"
#include "pland/mod/ModEntry.h"
#include "pland/utils/JSON.h"
#include "pland/utils/McUtils.h"
#include <cstdint>
#include <stack>
#include <string>
#include <vector>


using namespace ll::form;

namespace land {

void LandManagerGUI::impl(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地不存在"_trf(player));
        return;
    }

    auto fm = BackSimpleForm<>::make();
    fm.setTitle(PLUGIN_NAME + ("| 领地管理 [{}]"_trf(player, land->getLandID())));

    string subContent;
    if (land->isParentLand()) {
        subContent = "下属子领地: {}"_trf(player, land->getSubLands().size());
    } else if (land->isMixLand()) {
        subContent = "下属子领地: {}\n父领地ID: {}\n父领地名称: {}"_trf(
            player,
            land->getSubLands().size(),
            land->mParentLandID,
            land->getParentLand() ? land->getParentLand()->getLandName() : "nullptr"
        );
    } else {
        subContent = "父领地ID: {}\n父领地名称: {}"_trf(
            player,
            land->mParentLandID,
            land->getParentLand() ? land->getParentLand()->getLandName() : "nullptr"
        );
    }

    fm.setContent("领地: {}\n类型: {}\n大小: {}x{}x{} = {}\n范围: {}\n\n{}"_trf(
        player,
        land->getLandName(),
        land->is3DLand() ? "3D" : "2D",
        land->getLandPos().getDepth(),
        land->getLandPos().getWidth(),
        land->getLandPos().getHeight(),
        land->getLandPos().getVolume(),
        land->getLandPos().toString(),
        subContent
    ));


    fm.appendButton("编辑权限"_trf(player), "textures/ui/sidebar_icons/promotag", "path", [land](Player& pl) {
        EditLandPermGui::impl(pl, land);
    });
    fm.appendButton("修改成员"_trf(player), "textures/ui/FriendsIcon", "path", [land](Player& pl) {
        EditLandMemberGui::impl(pl, land);
    });
    fm.appendButton("修改领地名称"_trf(player), "textures/ui/book_edit_default", "path", [land](Player& pl) {
        EditLandNameGui::impl(pl, land);
    });
    fm.appendButton("修改领地描述"_trf(player), "textures/ui/book_edit_default", "path", [land](Player& pl) {
        EditLandDescGui::impl(pl, land);
    });

    // 开启了领地传送功能，或者玩家是领地管理员
    if (Config::cfg.land.landTp || PLand::getInstance().isOperator(player.getUuid().asString())) {
        fm.appendButton("传送到领地"_trf(player), "textures/ui/icon_recipe_nature", "path", [id](Player& pl) {
            LandTeleportGUI::impl(pl, id);
        });

        // 如果玩家在领地内，则显示设置传送点按钮
        if (land->mPos.hasPos(player.getPosition())) {
            fm.appendButton("设置传送点"_trf(player), "textures/ui/Add-Ons_Nav_Icon36x36", "path", [land](Player& pl) {
                land->mTeleportPos = pl.getPosition();
                mc_utils::sendText(pl, "领地传送点已设置!"_trf(pl));
            });
        }
    }

    fm.appendButton("领地过户"_trf(player), "textures/ui/sidebar_icons/my_characters", "path", [land](Player& pl) {
        EditLandOwnerGui::impl(pl, land);
    });

    if (land->isOrdinaryLand()) {
        fm.appendButton("重新选区"_trf(player), "textures/ui/anvil_icon", "path", [land](Player& pl) {
            ReSelectLandGui::impl(pl, land);
        });
    }

    fm.appendButton("删除领地"_trf(player), "textures/ui/icon_trash", "path", [land](Player& pl) {
        DeleteLandGui::impl(pl, land);
    });

    fm.sendTo(player);
}
void LandManagerGUI::EditLandPermGui::impl(Player& player, LandData_sptr const& ptr) {
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


void LandManagerGUI::DeleteLandGui::impl(Player& player, LandData_sptr const& ptr) {
    if (ptr->isOrdinaryLand()) {
        _deleteOrdinaryLandImpl(player, ptr);
    } else if (ptr->isSubLand()) {
        _deleteSubLandImpl(player, ptr);
    } else if (ptr->isParentLand()) {
        _deleteParentLandImpl(player, ptr);
    } else if (ptr->isMixLand()) {
        _deleteMixLandImpl(player, ptr);
    }
}
void LandManagerGUI::DeleteLandGui::recursionCalculationRefoundPrice(int& refundPrice, LandData_sptr const& ptr) {
    std::stack<LandData_sptr> stack;
    stack.push(ptr);

    while (!stack.empty()) {
        LandData_sptr current = stack.top();
        stack.pop();

        refundPrice += PriceCalculate::calculateRefundsPrice(current->mOriginalBuyPrice, Config::cfg.land.refundRate);

        if (current->hasSubLand()) {
            for (auto& subLand : current->getSubLands()) {
                stack.push(subLand);
            }
        }
    }
}

#define DELETE_LAND_GUI_REMOVE_DEFAULT_LAND_IMPL(FN)                                                                   \
    int       price = PriceCalculate::calculateRefundsPrice(ptr->mOriginalBuyPrice, Config::cfg.land.refundRate);      \
    ModalForm fm(                                                                                                      \
        PLUGIN_NAME + " | 确认删除?"_trf(player),                                                                      \
        "您确定要删除领地 {} 吗?\n删除领地后，您将获得 {} 金币的退款。\n此操作不可逆,请谨慎操作!"_trf(                 \
            player,                                                                                                    \
            ptr->getLandName(),                                                                                        \
            price                                                                                                      \
        ),                                                                                                             \
        "确认"_trf(player),                                                                                            \
        "返回"_trf(player)                                                                                             \
    );                                                                                                                 \
    fm.sendTo(player, [ptr, price](Player& pl, ModalFormResult const& res, FormCancelReason) {                         \
        if (!res) {                                                                                                    \
            return;                                                                                                    \
        }                                                                                                              \
        if (!(bool)res.value()) {                                                                                      \
            LandManagerGUI::impl(pl, ptr->getLandID());                                                                \
            return;                                                                                                    \
        }                                                                                                              \
        PlayerDeleteLandBeforeEvent ev(pl, ptr->getLandID(), price);                                                   \
        ll::event::EventBus::getInstance().publish(ev);                                                                \
        if (ev.isCancelled()) {                                                                                        \
            return;                                                                                                    \
        }                                                                                                              \
        auto& economy = EconomySystem::getInstance();                                                                  \
        if (!economy.add(pl, price)) {                                                                                 \
            mc_utils::sendText(pl, "经济系统异常，操作失败"_trf(pl));                                                  \
            return;                                                                                                    \
        }                                                                                                              \
        auto result = FN(ptr);                                                                                         \
        if (result.has_value() && result.value()) {                                                                    \
            auto handle = DrawHandleManager::getInstance().getOrCreateHandle(pl);                                      \
            handle->remove(ptr->getLandID());                                                                          \
            PlayerDeleteLandAfterEvent evAfter(pl, ptr->getLandID());                                                  \
            ll::event::EventBus::getInstance().publish(evAfter);                                                       \
            mc_utils::sendText(pl, "删除领地成功!"_trf(pl));                                                           \
        } else {                                                                                                       \
            economy.reduce(pl, price);                                                                                 \
            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "删除领地失败，原因: {}"_trf(pl, result.error()));       \
        }                                                                                                              \
    });

#define DELETE_LAND_GUI_REMOVE_LAND_AND_SUB_LANDS_IMPL(pl, ptr)                                                        \
    int refundPrice = 0;                                                                                               \
    recursionCalculationRefoundPrice(refundPrice, ptr);                                                                \
    std::vector<LandID>       landIdsToRemove;                                                                         \
    std::stack<LandData_sptr> stack;                                                                                   \
    if (ptr) {                                                                                                         \
        stack.push(ptr);                                                                                               \
        while (!stack.empty()) {                                                                                       \
            LandData_sptr current = stack.top();                                                                       \
            stack.pop();                                                                                               \
            if (current) {                                                                                             \
                landIdsToRemove.push_back(current->getLandID());                                                       \
                if (current->hasSubLand()) {                                                                           \
                    for (auto& subLand : current->getSubLands()) {                                                     \
                        if (subLand) stack.push(subLand);                                                              \
                    }                                                                                                  \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    }                                                                                                                  \
    PlayerDeleteLandBeforeEvent ev(pl, ptr ? ptr->getLandID() : -1, refundPrice);                                      \
    ll::event::EventBus::getInstance().publish(ev);                                                                    \
    if (ev.isCancelled()) {                                                                                            \
        return;                                                                                                        \
    }                                                                                                                  \
    auto& economy = EconomySystem::getInstance();                                                                      \
    if (!economy.add(pl, refundPrice)) {                                                                               \
        mc_utils::sendText(pl, "经济系统异常，操作失败"_trf(pl));                                                      \
        return;                                                                                                        \
    }                                                                                                                  \
    auto mainLandId = ptr ? ptr->getLandID() : -1;                                                                     \
    auto result     = PLand::getInstance().removeLandAndSubLands(ptr);                                                 \
    if (result.has_value() && result.value()) {                                                                        \
        /* Remove draw for all collected IDs */                                                                        \
        auto handle = DrawHandleManager::getInstance().getOrCreateHandle(pl);                                          \
        for (const auto& idToRemove : landIdsToRemove) {                                                               \
            handle->remove(idToRemove);                                                                                \
        }                                                                                                              \
        PlayerDeleteLandAfterEvent evAfter(pl, mainLandId);                                                            \
        ll::event::EventBus::getInstance().publish(evAfter);                                                           \
        mc_utils::sendText(pl, "删除领地成功!"_trf(pl));                                                               \
    } else {                                                                                                           \
        economy.reduce(pl, refundPrice);                                                                               \
        mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "删除领地失败，原因: {}"_trf(pl, result.error()));           \
    }

#define DELETE_LAND_GUI_REMOVE_LAND_AND_PROMOTE_SUB_LANDS_IMPL(pl, ptr, fn)                                            \
    int refundPrice = PriceCalculate::calculateRefundsPrice(ptr->mOriginalBuyPrice, Config::cfg.land.refundRate);      \
    PlayerDeleteLandBeforeEvent ev(pl, ptr->getLandID(), refundPrice);                                                 \
    ll::event::EventBus::getInstance().publish(ev);                                                                    \
    if (ev.isCancelled()) {                                                                                            \
        return;                                                                                                        \
    }                                                                                                                  \
    auto& economy = EconomySystem::getInstance();                                                                      \
    if (!economy.add(pl, refundPrice)) {                                                                               \
        mc_utils::sendText(pl, "经济系统异常，操作失败"_trf(pl));                                                      \
        return;                                                                                                        \
    }                                                                                                                  \
    auto landId = ptr->getLandID();                                                                                    \
    auto result = fn(ptr);                                                                                             \
    if (result.has_value() && result.value()) {                                                                        \
        auto handle = DrawHandleManager::getInstance().getOrCreateHandle(pl);                                          \
        handle->remove(landId);                                                                                        \
        PlayerDeleteLandAfterEvent evAfter(pl, landId);                                                                \
        ll::event::EventBus::getInstance().publish(evAfter);                                                           \
        mc_utils::sendText(pl, "删除领地成功!"_trf(pl));                                                               \
    } else {                                                                                                           \
        economy.reduce(pl, refundPrice);                                                                               \
        mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "删除领地失败，原因: {}"_trf(pl, result.error()));           \
    }

void LandManagerGUI::DeleteLandGui::_deleteOrdinaryLandImpl(Player& player, LandData_sptr const& ptr) {
    DELETE_LAND_GUI_REMOVE_DEFAULT_LAND_IMPL(PLand::getInstance().removeOrdinaryLand);
}
void LandManagerGUI::DeleteLandGui::_deleteSubLandImpl(Player& player, LandData_sptr const& ptr) {
    DELETE_LAND_GUI_REMOVE_DEFAULT_LAND_IMPL(PLand::getInstance().removeSubLand);
}
#undef DELETE_LAND_GUI_REMOVE_DEFAULT_LAND_IMPL

void LandManagerGUI::DeleteLandGui::_deleteParentLandImpl(Player& player, LandData_sptr const& ptr) {
    auto fm = BackSimpleForm<>::make<LandManagerGUI::impl>(ptr->getLandID());
    fm.setTitle(PLUGIN_NAME + "| 删除领地 & 父领地"_trf(player));
    fm.setContent(
        "您当前操作的的是父领地\n当前领地下有 {} 个子领地\n您确定要删除领地吗?"_trf(player, ptr->getSubLands().size())
    );

    fm.appendButton("删除当前领地和子领地"_trf(player), [ptr](Player& pl) {
        DELETE_LAND_GUI_REMOVE_LAND_AND_SUB_LANDS_IMPL(pl, ptr);
    });
    fm.appendButton("删除当前领地并提升子领地为父领地"_trf(player), [ptr](Player& pl) {
        DELETE_LAND_GUI_REMOVE_LAND_AND_PROMOTE_SUB_LANDS_IMPL(
            pl,
            ptr,
            PLand::getInstance().removeLandAndPromoteSubLands
        );
    });

    fm.sendTo(player);
}
void LandManagerGUI::DeleteLandGui::_deleteMixLandImpl(Player& player, LandData_sptr const& ptr) {
    auto fm = BackSimpleForm<>::make<LandManagerGUI::impl>(ptr->getLandID());
    fm.setTitle(PLUGIN_NAME + "| 删除领地 & 混合领地"_trf(player));
    fm.setContent(
        "您当前操作的的是混合领地\n当前领地下有 {} 个子领地\n您确定要删除领地吗?"_trf(player, ptr->getSubLands().size())
    );

    fm.appendButton("删除当前领地和子领地"_trf(player), [ptr](Player& pl) {
        DELETE_LAND_GUI_REMOVE_LAND_AND_SUB_LANDS_IMPL(pl, ptr);
    });
    fm.appendButton("删除当前领地并移交子领地给父领地"_trf(player), [ptr](Player& pl) {
        DELETE_LAND_GUI_REMOVE_LAND_AND_PROMOTE_SUB_LANDS_IMPL(
            pl,
            ptr,
            PLand::getInstance().removeLandAndTransferSubLands
        );
    });

    fm.sendTo(player);
}
#undef DELETE_LAND_GUI_REMOVE_LAND_AND_PROMOTE_SUB_LANDS_IMPL
#undef DELETE_LAND_GUI_REMOVE_LAND_AND_SUB_LANDS_IMPL


void LandManagerGUI::EditLandNameGui::impl(Player& player, LandData_sptr const& ptr) {
    EditStringUtilGUI::sendTo(
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
void LandManagerGUI::EditLandDescGui::impl(Player& player, LandData_sptr const& ptr) {
    EditStringUtilGUI::sendTo(
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
void LandManagerGUI::EditLandOwnerGui::impl(Player& player, LandData_sptr const& ptr) {
    ChoosePlayerUtilGUI::sendTo(player, [ptr](Player& self, Player* target) {
        if (!target) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "目标玩家已离线，无法继续操作!"_trf(self));
            return;
        }

        if (self.getUuid() == target->getUuid()) {
            mc_utils::sendText(self, "不能将领地转让给自己, 左手倒右手哦!"_trf(self));
            return;
        }

        LandOwnerChangeBeforeEvent ev(self, *target, ptr->getLandID());
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm fm(
            PLUGIN_NAME + " | 确认转让?"_trf(self),
            "您确定要将领地转让给 {} 吗?\n转让后，您将失去此领地的权限。\n此操作不可逆,请谨慎操作!"_trf(
                self,
                target->getRealName()
            ),
            "确认"_trf(self),
            "返回"_trf(self)
        );
        fm.sendTo(
            self,
            [ptr, weak = target->getWeakEntity()](Player& self, ModalFormResult const& res, FormCancelReason) {
                if (!res) {
                    return;
                }
                Player* target = weak.tryUnwrap<Player>();
                if (!target) {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(self, "目标玩家已离线，无法继续操作!"_trf(self));
                    return;
                }

                if (!(bool)res.value()) {
                    LandManagerGUI::impl(self, ptr->getLandID());
                    return;
                }

                if (ptr->setLandOwner(target->getUuid().asString())) {
                    mc_utils::sendText(self, "领地已转让给 {}"_trf(self, target->getRealName()));
                    mc_utils::sendText(
                        target,
                        "您已成功接手来自 \"{}\" 的领地 \"{}\""_trf(self, self.getRealName(), ptr->getLandName())
                    );

                    LandOwnerChangeAfterEvent ev(self, *target, ptr->getLandID());
                    ll::event::EventBus::getInstance().publish(ev);
                } else {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(self, "领地转让失败!"_trf(self));
                }
            }
        );
    });
}
void LandManagerGUI::ReSelectLandGui::impl(Player& player, LandData_sptr const& ptr) {
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
            LandManagerGUI::impl(self, ptr->getLandID());
            return;
        }

        auto selector = std::make_unique<LandReSelector>(self, ptr);
        if (!SelectorManager::getInstance().start(std::move(selector))) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "选区开启失败，当前存在未完成的选区任务"_trf(self));
        }
    });
}


void LandManagerGUI::EditLandMemberGui::impl(Player& player, LandData_sptr ptr) {
    auto fm = BackSimpleForm<>::make<LandManagerGUI::impl>(ptr->getLandID());

    fm.appendButton("添加成员"_trf(player), "textures/ui/color_plus", "path", [ptr](Player& self) {
        AddMemberGui::impl(self, ptr);
    });

    auto& infos = ll::service::PlayerInfo::getInstance();
    for (auto& member : ptr->getLandMembers()) {
        auto i = infos.fromUuid(UUIDm::fromString(member));
        if (!i) {
            mod::ModEntry::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
        }

        fm.appendButton(i.has_value() ? i->name : member, [member, ptr](Player& self) {
            RemoveMemberGui::impl(self, ptr, member);
        });
    }

    fm.sendTo(player);
}
void LandManagerGUI::EditLandMemberGui::AddMemberGui::impl(Player& player, LandData_sptr ptr) {
    ChoosePlayerUtilGUI::sendTo(
        player,
        [ptr](Player& self, Player* target) {
            if (!target) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(self, "目标玩家已离线，无法继续操作!"_trf(self));
                return;
            }

            if (self.getUuid() == target->getUuid() && !PLand::getInstance().isOperator(self.getUuid().asString())) {
                mc_utils::sendText(self, "不能添加自己为领地成员哦!"_trf(self));
                return;
            }

            LandMemberChangeBeforeEvent ev(self, target->getUuid().asString(), ptr->getLandID(), true);
            ll::event::EventBus::getInstance().publish(ev);
            if (ev.isCancelled()) {
                return;
            }

            ModalForm fm(
                PLUGIN_NAME + " | 添加成员"_trf(self),
                "您确定要添加 {} 为领地成员吗?"_trf(self, target->getRealName()),
                "确认"_trf(self),
                "返回"_trf(self)
            );
            fm.sendTo(
                self,
                [ptr, weak = target->getWeakEntity()](Player& self, ModalFormResult const& res, FormCancelReason) {
                    if (!res) {
                        return;
                    }
                    Player* target = weak.tryUnwrap<Player>();
                    if (!target) {
                        mc_utils::sendText<mc_utils::LogLevel::Error>(self, "目标玩家已离线，无法继续操作!"_trf(self));
                        return;
                    }

                    if (!(bool)res.value()) {
                        EditLandMemberGui::impl(self, ptr);
                        return;
                    }

                    if (ptr->isLandMember(target->getUuid().asString())) {
                        mc_utils::sendText(self, "该玩家已经是领地成员, 请不要重复添加哦!"_trf(self));
                        return;
                    }

                    if (ptr->addLandMember(target->getUuid().asString())) {
                        mc_utils::sendText(self, "添加成功!"_trf(self));

                        LandMemberChangeAfterEvent ev(self, target->getUuid().asString(), ptr->getLandID(), true);
                        ll::event::EventBus::getInstance().publish(ev);
                    } else {
                        mc_utils::sendText(self, "添加失败!"_trf(self));
                    }
                }
            );
        },
        BackSimpleForm<>::makeCallback<EditLandMemberGui::impl>(ptr)
    );
}
void LandManagerGUI::EditLandMemberGui::RemoveMemberGui::impl(Player& player, LandData_sptr ptr, UUIDs member) {
    LandMemberChangeBeforeEvent ev(player, member, ptr->getLandID(), false);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto info = ll::service::PlayerInfo::getInstance().fromUuid(UUIDm::fromString(member));
    if (!info) {
        mod::ModEntry::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
    }

    ModalForm fm(
        PLUGIN_NAME + " | 移除成员"_trf(player),
        "您确定要移除成员 \"{}\" 吗?"_trf(player, info.has_value() ? info->name : member),
        "确认"_trf(player),
        "返回"_trf(player)
    );
    fm.sendTo(player, [ptr, member](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        if (!(bool)res.value()) {
            EditLandMemberGui::impl(self, ptr);
            return;
        }

        if (ptr->removeLandMember(member)) {
            mc_utils::sendText(self, "移除成功!"_trf(self));

            LandMemberChangeAfterEvent ev(self, member, ptr->getLandID(), false);
            ll::event::EventBus::getInstance().publish(ev);
        } else {
            mc_utils::sendText(self, "移除失败!"_trf(self));
        }
    });
}


} // namespace land
