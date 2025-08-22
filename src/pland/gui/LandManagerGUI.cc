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
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/economy/EconomySystem.h"
#include "pland/economy/PriceCalculate.h"
#include "pland/gui/CommonUtilGUI.h"
#include "pland/gui/common/EditLandPermTableUtilGUI.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/infra/Config.h"
#include "pland/infra/DrawHandleManager.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/land/Land.h"
#include "pland/land/LandContext.h"
#include "pland/land/LandCreateValidator.h"
#include "pland/land/LandEvent.h"
#include "pland/land/LandRegistry.h"
#include "pland/land/StorageLayerError.h"
#include "pland/selector/ChangeLandRangeSelector.h"
#include "pland/selector/SelectorManager.h"
#include "pland/utils/JSON.h"
#include "pland/utils/McUtils.h"
#include <cstdint>
#include <stack>
#include <string>
#include <vector>


using namespace ll::form;

namespace land {


void LandManagerGUI::sendMainMenu(Player& player, SharedLand land) {
    auto fm = BackSimpleForm<>::make();
    fm.setTitle(PLUGIN_NAME + ("| 领地管理 [{}]"_trf(player, land->getId())));

    string subContent;
    if (land->isParentLand()) {
        subContent = "下属子领地: {}"_trf(player, land->getSubLands().size());
    } else if (land->isMixLand()) {
        subContent = "下属子领地: {}\n父领地ID: {}\n父领地名称: {}"_trf(
            player,
            land->getSubLands().size(),
            land->getParentLand()->getId(),
            land->getParentLand() ? land->getParentLand()->getName() : "nullptr"
        );
    } else {
        subContent = "父领地ID: {}\n父领地名称: {}"_trf(
            player,
            land->hasParentLand() ? (std::to_string(land->getParentLand()->getId())) : "nullptr",
            land->hasParentLand() ? land->getParentLand()->getName() : "nullptr"
        );
    }

    fm.setContent(
        "领地: {}\n类型: {}\n大小: {}x{}x{} = {}\n范围: {}\n\n{}"_trf(
            player,
            land->getName(),
            land->is3D() ? "3D" : "2D",
            land->getAABB().getDepth(),
            land->getAABB().getWidth(),
            land->getAABB().getHeight(),
            land->getAABB().getVolume(),
            land->getAABB().toString(),
            subContent
        )
    );


    fm.appendButton("编辑权限"_trf(player), "textures/ui/sidebar_icons/promotag", "path", [land](Player& pl) {
        sendEditLandPermGUI(pl, land);
    });
    fm.appendButton("修改成员"_trf(player), "textures/ui/FriendsIcon", "path", [land](Player& pl) {
        sendChangeMemberGUI(pl, land);
    });
    fm.appendButton("修改领地名称"_trf(player), "textures/ui/book_edit_default", "path", [land](Player& pl) {
        sendEditLandNameGUI(pl, land);
    });
    fm.appendButton("修改领地描述"_trf(player), "textures/ui/book_edit_default", "path", [land](Player& pl) {
        sendEditLandDescGUI(pl, land);
    });

    // 开启了领地传送功能，或者玩家是领地管理员
    if (Config::cfg.land.landTp || PLand::getInstance().getLandRegistry()->isOperator(player.getUuid().asString())) {
        fm.appendButton("传送到领地"_trf(player), "textures/ui/icon_recipe_nature", "path", [land](Player& pl) {
            LandTeleportGUI::impl(pl, land);
        });

        // 如果玩家在领地内，则显示设置传送点按钮
        if (land->getAABB().hasPos(player.getPosition())) {
            fm.appendButton("设置传送点"_trf(player), "textures/ui/Add-Ons_Nav_Icon36x36", "path", [land](Player& pl) {
                land->setTeleportPos(LandPos::make(pl.getPosition()));
                mc_utils::sendText(pl, "领地传送点已设置!"_trf(pl));
            });
        }
    }

    fm.appendButton("领地过户"_trf(player), "textures/ui/sidebar_icons/my_characters", "path", [land](Player& pl) {
        sendTransferLandGUI(pl, land);
    });

    if (land->isOrdinaryLand()) {
        fm.appendButton("重新选区"_trf(player), "textures/ui/anvil_icon", "path", [land](Player& pl) {
            sendChangLandRangeGUI(pl, land);
        });
    }

    fm.appendButton("删除领地"_trf(player), "textures/ui/icon_trash", "path", [land](Player& pl) {
        sendRemoveConfirmGUI(pl, land);
    });

    fm.sendTo(player);
}

void LandManagerGUI::sendEditLandPermGUI(Player& player, SharedLand const& ptr) {
    EditLandPermTableUtilGUI::sendTo(player, ptr->getPermTable(), [ptr](Player& self, LandPermTable newTable) {
        ptr->setPermTable(newTable);
        mc_utils::sendText(self, "权限表已更新"_trf(self));
    });
}


void LandManagerGUI::sendRemoveConfirmGUI(Player& player, SharedLand const& ptr) {
    if (ptr->isOrdinaryLand() || ptr->isSubLand()) {
        _implRemoveWithOrdinaryOrSubLandGUI(player, ptr);
    } else if (ptr->isParentLand()) {
        _implRemoveParentLandGUI(player, ptr);
    } else if (ptr->isMixLand()) {
        _implRemoveMixLandGUI(player, ptr);
    }
}

void LandManagerGUI::_implRemoveWithOrdinaryOrSubLandGUI(Player& player, SharedLand const& ptr) {
    int price = static_cast<int>(Land::calculatePriceRecursively(ptr, [](SharedLand const& land, llong& price) {
        price += PriceCalculate::calculateRefundsPrice(land->getOriginalBuyPrice(), Config::cfg.land.refundRate);
        return true;
    }));

    ModalForm(
        PLUGIN_NAME + " | 确认删除?"_trf(player),
        "您确定要删除领地 {} 吗?\n删除领地后，您将获得 {} 金币的退款。\n此操作不可逆,请谨慎操作!"_trf(
            player,
            ptr->getName(),
            price
        ),
        "确认"_trf(player),
        "返回"_trf(player)
    )
        .sendTo(player, [ptr, price](Player& pl, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }
            if (!(bool)res.value()) {
                sendMainMenu(pl, ptr);
                return;
            }

            PlayerDeleteLandBeforeEvent ev(pl, ptr->getId(), price);
            ll::event::EventBus::getInstance().publish(ev);
            if (ev.isCancelled()) {
                return;
            }

            auto& economy = EconomySystem::getInstance();
            if (!economy->add(pl, price)) {
                return;
            }

            auto result = ptr->isSubLand() ? PLand::getInstance().getLandRegistry()->removeSubLand(ptr)
                                           : PLand::getInstance().getLandRegistry()->removeOrdinaryLand(ptr);
            if (!result) {
                economy->reduce(pl, price);
                return;
            }

            auto handle = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(pl);
            handle->remove(ptr);

            ll::event::EventBus::getInstance().publish(PlayerDeleteLandAfterEvent{pl, ptr->getId()});
        });
}

void LandManagerGUI::_implRemoveParentLandGUI(Player& player, SharedLand const& ptr) {
    auto fm = BackSimpleForm<>::make<LandManagerGUI::sendMainMenu>(ptr);
    fm.setTitle(PLUGIN_NAME + "| 删除领地 & 父领地"_trf(player));
    fm.setContent(
        "您当前操作的的是父领地\n当前领地下有 {} 个子领地\n您确定要删除领地吗?"_trf(player, ptr->getSubLands().size())
    );

    fm.appendButton("删除当前领地和子领地"_trf(player), [ptr](Player& pl) {
        int price = static_cast<int>(Land::calculatePriceRecursively(ptr, [](SharedLand const& land, llong& price) {
            price += PriceCalculate::calculateRefundsPrice(land->getOriginalBuyPrice(), Config::cfg.land.refundRate);
            return true;
        }));

        auto mainLandId = ptr->getId();

        PlayerDeleteLandBeforeEvent ev(pl, mainLandId, price);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        auto& economy = EconomySystem::getInstance();
        if (!economy->add(pl, price)) {
            return;
        }

        auto subLands = ptr->getSelfAndDescendants();
        auto handle   = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(pl);
        for (const auto& ld : subLands) {
            handle->remove(ld);
        }

        auto result = PLand::getInstance().getLandRegistry()->removeLandAndSubLands(ptr);
        if (!result) {
            economy->reduce(pl, price);
            return;
        }

        ll::event::EventBus::getInstance().publish(PlayerDeleteLandAfterEvent{pl, mainLandId});
    });
    fm.appendButton("删除当前领地并提升子领地为父领地"_trf(player), [ptr](Player& pl) {
        int refundPrice =
            PriceCalculate::calculateRefundsPrice(ptr->getOriginalBuyPrice(), Config::cfg.land.refundRate);

        PlayerDeleteLandBeforeEvent ev(pl, ptr->getId(), refundPrice);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        auto& economy = EconomySystem::getInstance();
        if (!economy->add(pl, refundPrice)) {
            return;
        }

        auto result = PLand::getInstance().getLandRegistry()->removeLandAndPromoteSubLands(ptr);
        if (!result) {
            economy->reduce(pl, refundPrice);
            return;
        }


        PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(pl)->remove(ptr);
        ll::event::EventBus::getInstance().publish(PlayerDeleteLandAfterEvent{pl, ptr->getId()});
    });

    fm.sendTo(player);
}
void LandManagerGUI::_implRemoveMixLandGUI(Player& player, SharedLand const& ptr) {
    auto fm = BackSimpleForm<>::make<LandManagerGUI::sendMainMenu>(ptr);
    fm.setTitle(PLUGIN_NAME + "| 删除领地 & 混合领地"_trf(player));
    fm.setContent(
        "您当前操作的的是混合领地\n当前领地下有 {} 个子领地\n您确定要删除领地吗?"_trf(player, ptr->getSubLands().size())
    );

    fm.appendButton("删除当前领地和子领地"_trf(player), [ptr](Player& pl) {
        int price = static_cast<int>(Land::calculatePriceRecursively(ptr, [](SharedLand const& land, llong& price) {
            price += PriceCalculate::calculateRefundsPrice(land->getOriginalBuyPrice(), Config::cfg.land.refundRate);
            return true;
        }));

        auto mainLandId = ptr->getId();

        PlayerDeleteLandBeforeEvent ev(pl, mainLandId, price);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        auto& economy = EconomySystem::getInstance();
        if (!economy->add(pl, price)) {
            return;
        }

        auto subLands = ptr->getSelfAndDescendants();
        auto handle   = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(pl);
        for (const auto& ld : subLands) {
            handle->remove(ld);
        }

        auto result = PLand::getInstance().getLandRegistry()->removeLandAndSubLands(ptr);
        if (!result) {
            economy->reduce(pl, price);
            return;
        }

        ll::event::EventBus::getInstance().publish(PlayerDeleteLandAfterEvent{pl, mainLandId});
    });
    fm.appendButton("删除当前领地并移交子领地给父领地"_trf(player), [ptr](Player& pl) {
        int refundPrice =
            PriceCalculate::calculateRefundsPrice(ptr->getOriginalBuyPrice(), Config::cfg.land.refundRate);

        PlayerDeleteLandBeforeEvent ev(pl, ptr->getId(), refundPrice);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        auto& economy = EconomySystem::getInstance();
        if (!economy->add(pl, refundPrice)) {
            return;
        }

        auto result = PLand::getInstance().getLandRegistry()->removeLandAndTransferSubLands(ptr);
        if (!result) {
            economy->reduce(pl, refundPrice);
            return;
        }

        PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(pl)->remove(ptr);
        ll::event::EventBus::getInstance().publish(PlayerDeleteLandAfterEvent{pl, ptr->getId()});
    });

    fm.sendTo(player);
}

void LandManagerGUI::sendEditLandNameGUI(Player& player, SharedLand const& ptr) {
    EditStringUtilGUI::sendTo(
        player,
        "修改领地名称"_trf(player),
        "请输入新的领地名称"_trf(player),
        ptr->getName(),
        [ptr](Player& pl, string result) {
            ptr->setName(result);
            mc_utils::sendText(pl, "领地名称已更新!"_trf(pl));
        }
    );
}
void LandManagerGUI::sendEditLandDescGUI(Player& player, SharedLand const& ptr) {
    EditStringUtilGUI::sendTo(
        player,
        "修改领地描述"_trf(player),
        "请输入新的领地描述"_trf(player),
        ptr->getDescribe(),
        [ptr](Player& pl, string result) {
            ptr->setDescribe(result);
            mc_utils::sendText(pl, "领地描述已更新!"_trf(pl));
        }
    );
}
void LandManagerGUI::sendTransferLandGUI(Player& player, SharedLand const& ptr) {
    auto fm = BackSimpleForm<>::make<LandManagerGUI::sendMainMenu>(ptr);
    fm.setTitle(PLUGIN_NAME + "| 转让领地"_trf(player));

    fm.appendButton(
        "转让给在线玩家"_trf(player),
        "textures/ui/sidebar_icons/my_characters",
        "path",
        [ptr](Player& self) {
            ChooseOnlinePlayerUtilGUI::sendTo(self, [ptr](Player& self, Player& target) {
                if (self.getUuid() == target.getUuid()) {
                    mc_utils::sendText(self, "不能将领地转让给自己, 左手倒右手哦!"_trf(self));
                    return;
                }

                if (auto res = LandCreateValidator::isPlayerLandCountLimitExceeded(target.getUuid().asString()); !res) {
                    LandCreateValidator::sendErrorMessage(self, res.error());
                    return;
                }

                LandOwnerChangeBeforeEvent ev(self, target.getUuid().asString(), ptr->getId());
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
                fm.sendTo(
                    self,
                    [ptr, weak = target.getWeakEntity()](Player& self, ModalFormResult const& res, FormCancelReason) {
                        if (!res) {
                            return;
                        }
                        Player* target = weak.tryUnwrap<Player>();
                        if (!target) {
                            mc_utils::sendText<mc_utils::LogLevel::Error>(
                                self,
                                "目标玩家已离线，无法继续操作!"_trf(self)
                            );
                            return;
                        }

                        if (!(bool)res.value()) {
                            LandManagerGUI::sendMainMenu(self, ptr);
                            return;
                        }

                        ptr->setOwner(target->getUuid().asString());

                        mc_utils::sendText(self, "领地已转让给 {}"_trf(self, target->getRealName()));
                        mc_utils::sendText(
                            target,
                            "您已成功接手来自 \"{}\" 的领地 \"{}\""_trf(self, self.getRealName(), ptr->getName())
                        );

                        LandOwnerChangeAfterEvent ev(self, target->getUuid().asString(), ptr->getId());
                        ll::event::EventBus::getInstance().publish(ev);
                    }
                );
            });
        }
    );

    fm.appendButton(
        "转让给离线玩家"_trf(player),
        "textures/ui/sidebar_icons/my_characters",
        "path",
        [ptr](Player& self) { _sendTransferLandToOfflinePlayerGUI(self, ptr); }
    );

    fm.sendTo(player);
}

void LandManagerGUI::_sendTransferLandToOfflinePlayerGUI(Player& player, SharedLand const& ptr) {
    CustomForm fm(PLUGIN_NAME + " | 转让给离线玩家"_trf(player));
    fm.appendInput("playerName", "请输入离线玩家名称"_trf(player), "玩家名称");
    fm.sendTo(player, [ptr](Player& self, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        auto playerName = std::get<std::string>(res->at("playerName"));
        if (playerName.empty()) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "玩家名称不能为空!"_trf(self));
            sendTransferLandGUI(self, ptr);
            return;
        }

        auto playerInfo = ll::service::PlayerInfo::getInstance().fromName(playerName);
        if (!playerInfo) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "未找到该玩家信息，请检查名称是否正确!"_trf(self));
            sendTransferLandGUI(self, ptr);
            return;
        }

        auto targetUuid = playerInfo->uuid.asString();

        if (self.getUuid().asString() == targetUuid) {
            mc_utils::sendText(self, "不能将领地转让给自己, 左手倒右手哦!"_trf(self));
            sendTransferLandGUI(self, ptr);
            return;
        }

        if (auto res = LandCreateValidator::isPlayerLandCountLimitExceeded(targetUuid); !res) {
            LandCreateValidator::sendErrorMessage(self, res.error());
            return;
        }

        LandOwnerChangeBeforeEvent ev(self, targetUuid, ptr->getId());
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm confirmFm(
            PLUGIN_NAME + " | 确认转让"_trf(self),
            "您确定要将领地转让给 {} 吗?\n转让后，您将失去此领地的权限。\n此操作不可逆,请谨慎操作!"_trf(
                self,
                playerName
            ),
            "确认"_trf(self),
            "返回"_trf(self)
        );
        confirmFm.sendTo(
            self,
            [ptr, targetUuid, playerName](Player& self, ModalFormResult const& confirmRes, FormCancelReason) {
                if (!confirmRes) {
                    return;
                }
                if (!(bool)confirmRes.value()) {
                    sendTransferLandGUI(self, ptr);
                    return;
                }

                ptr->setOwner(targetUuid);

                mc_utils::sendText(self, "领地已转让给 {}"_trf(self, playerName));
                // 离线玩家无法发送消息，所以只给操作者发送消息
                // mc_utils::sendText(
                //     target,
                //     "您已成功接手来自 \"{}\" 的领地 \"{}\""_trf(self, self.getRealName(), ptr->getName())
                // );

                LandOwnerChangeAfterEvent ev(self, targetUuid, ptr->getId());
                ll::event::EventBus::getInstance().publish(ev);
            }
        );
    });
}

void LandManagerGUI::sendChangLandRangeGUI(Player& player, SharedLand const& ptr) {
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
            LandManagerGUI::sendMainMenu(self, ptr);
            return;
        }

        auto manager = land::PLand::getInstance().getSelectorManager();
        if (manager->hasSelector(self.getUuid())) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "选区开启失败，当前存在未完成的选区任务"_trf(self));
            return;
        }

        auto selector = std::make_unique<ChangeLandRangeSelector>(self, ptr);
        manager->startSelection(std::move(selector));
    });
}


void LandManagerGUI::sendChangeMemberGUI(Player& player, SharedLand ptr) {
    auto fm = BackSimpleForm<>::make<LandManagerGUI::sendMainMenu>(ptr);

    fm.appendButton("添加在线成员"_trf(player), "textures/ui/color_plus", "path", [ptr](Player& self) {
        _sendAddMemberGUI(self, ptr);
    });
    fm.appendButton("添加离线成员"_trf(player), "textures/ui/color_plus", "path", [ptr](Player& self) {
        _sendAddOfflineMemberGUI(self, ptr);
    });

    auto& infos = ll::service::PlayerInfo::getInstance();
    for (auto& member : ptr->getMembers()) {
        auto i = infos.fromUuid(UUIDm::fromString(member));
        if (!i) {
            land::PLand::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
        }

        fm.appendButton(i.has_value() ? i->name : member, [member, ptr](Player& self) {
            _sendRemoveMemberGUI(self, ptr, member);
        });
    }

    fm.sendTo(player);
}
void LandManagerGUI::_sendAddMemberGUI(Player& player, SharedLand ptr) {
    ChooseOnlinePlayerUtilGUI::sendTo(
        player,
        [ptr](Player& self, Player& target) {
            if (self.getUuid() == target.getUuid()
                && !PLand::getInstance().getLandRegistry()->isOperator(self.getUuid().asString())) {
                mc_utils::sendText(self, "不能添加自己为领地成员哦!"_trf(self));
                return;
            }

            LandMemberChangeBeforeEvent ev(self, target.getUuid().asString(), ptr->getId(), true);
            ll::event::EventBus::getInstance().publish(ev);
            if (ev.isCancelled()) {
                return;
            }

            ModalForm fm(
                PLUGIN_NAME + " | 添加成员"_trf(self),
                "您确定要添加 {} 为领地成员吗?"_trf(self, target.getRealName()),
                "确认"_trf(self),
                "返回"_trf(self)
            );
            fm.sendTo(
                self,
                [ptr, weak = target.getWeakEntity()](Player& self, ModalFormResult const& res, FormCancelReason) {
                    if (!res) {
                        return;
                    }
                    Player* target = weak.tryUnwrap<Player>();
                    if (!target) {
                        mc_utils::sendText<mc_utils::LogLevel::Error>(self, "目标玩家已离线，无法继续操作!"_trf(self));
                        return;
                    }

                    if (!(bool)res.value()) {
                        sendChangeMemberGUI(self, ptr);
                        return;
                    }

                    if (ptr->isMember(target->getUuid().asString())) {
                        mc_utils::sendText(self, "该玩家已经是领地成员, 请不要重复添加哦!"_trf(self));
                        return;
                    }

                    ptr->addLandMember(target->getUuid().asString());
                    mc_utils::sendText(self, "添加成功!"_trf(self));

                    LandMemberChangeAfterEvent ev(self, target->getUuid().asString(), ptr->getId(), true);
                    ll::event::EventBus::getInstance().publish(ev);
                }
            );
        },
        BackSimpleForm<>::makeCallback<sendChangeMemberGUI>(ptr)
    );
}

void LandManagerGUI::_sendAddOfflineMemberGUI(Player& player, SharedLand ptr) {
    CustomForm fm(PLUGIN_NAME + " | 添加离线成员"_trf(player));
    fm.appendInput("playerName", "请输入离线玩家名称"_trf(player), "玩家名称");
    fm.sendTo(player, [ptr](Player& self, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        auto playerName = std::get<std::string>(res->at("playerName"));
        if (playerName.empty()) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "玩家名称不能为空!"_trf(self));
            sendChangeMemberGUI(self, ptr);
            return;
        }

        auto playerInfo = ll::service::PlayerInfo::getInstance().fromName(playerName);
        if (!playerInfo) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "未找到该玩家信息，请检查名称是否正确!"_trf(self));
            sendChangeMemberGUI(self, ptr);
            return;
        }

        auto targetUuid = playerInfo->uuid.asString();

        if (self.getUuid().asString() == targetUuid
            && !PLand::getInstance().getLandRegistry()->isOperator(self.getUuid().asString())) {
            mc_utils::sendText(self, "不能添加自己为领地成员哦!"_trf(self));
            sendChangeMemberGUI(self, ptr);
            return;
        }

        LandMemberChangeBeforeEvent ev(self, targetUuid, ptr->getId(), true);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm confirmFm(
            PLUGIN_NAME + " | 添加离线成员"_trf(self),
            "您确定要添加 {} 为领地成员吗?"_trf(self, playerName),
            "确认"_trf(self),
            "返回"_trf(self)
        );
        confirmFm.sendTo(
            self,
            [ptr, targetUuid, playerName](Player& self, ModalFormResult const& confirmRes, FormCancelReason) {
                if (!confirmRes) {
                    return;
                }
                if (!(bool)confirmRes.value()) {
                    sendChangeMemberGUI(self, ptr);
                    return;
                }

                if (ptr->isMember(targetUuid)) {
                    mc_utils::sendText(self, "该玩家已经是领地成员, 请不要重复添加哦!"_trf(self));
                    return;
                }

                ptr->addLandMember(targetUuid);
                mc_utils::sendText(self, "添加成功!"_trf(self));

                LandMemberChangeAfterEvent ev(self, targetUuid, ptr->getId(), true);
                ll::event::EventBus::getInstance().publish(ev);
            }
        );
    });
}

void LandManagerGUI::_sendRemoveMemberGUI(Player& player, SharedLand ptr, UUIDs member) {
    LandMemberChangeBeforeEvent ev(player, member, ptr->getId(), false);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto info = ll::service::PlayerInfo::getInstance().fromUuid(UUIDm::fromString(member));
    if (!info) {
        land::PLand::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
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
            sendChangeMemberGUI(self, ptr);
            return;
        }

        ptr->removeLandMember(member);
        mc_utils::sendText(self, "移除成功!"_trf(self));

        LandMemberChangeAfterEvent ev(self, member, ptr->getId(), false);
        ll::event::EventBus::getInstance().publish(ev);
    });
}


} // namespace land
