#include "pland/gui/LandManagerGUI.h"
#include "LandTeleportGUI.h"
#include "PermTableEditor.h"
#include "common/OnlinePlayerPicker.h"
#include "common/SimpleInputForm.h"

#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/service/PlayerInfo.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include "pland/PLand.h"
#include "pland/economy/EconomySystem.h"
#include "pland/gui/common/OnlinePlayerPicker.h"
#include "pland/gui/utils/BackUtils.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/service/LandHierarchyService.h"
#include "pland/service/LandManagementService.h"
#include "pland/service/LandPriceService.h"
#include "pland/service/LeasingService.h"
#include "pland/service/ServiceLocator.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/TimeUtils.h"

#include <string>
#include <utility>
#include <vector>


namespace land::gui {
using namespace ll::form;


void LandManagerGUI::sendMainMenu(Player& player, std::shared_ptr<Land> land) {
    auto fm = SimpleForm{};

    auto localeCode = player.getLocaleCode();
    fm.setTitle(("[PLand] | 领地管理 [ID:{}]"_trl(localeCode, land->getId())));

    auto& service = PLand::getInstance().getServiceLocator().getLandHierarchyService();

    std::string subContent;
    if (land->isParentLand()) {
        subContent = "下属子领地数量: {}"_trl(localeCode, land->getSubLandIDs().size());
    } else if (land->isMixLand()) {
        subContent = "下属子领地数量: {}\n父领地ID: {}\n父领地名称: {}"_trl(
            localeCode,
            land->getSubLandIDs().size(),
            service.getParent(land)->getId(),
            service.getParent(land)->getName()
        );
    } else {
        subContent = "父领地ID: {}\n父领地名称: {}"_trl(
            localeCode,
            land->hasParentLand() ? (std::to_string(service.getParent(land)->getId())) : "null",
            land->hasParentLand() ? service.getParent(land)->getName() : "null"
        );
    }

    std::string leaseContent;
    if (land->isLeased()) {


        auto state = land->getLeaseState();
        switch (state) {
        case LeaseState::None:
            break;
        case LeaseState::Active: {
            leaseContent =
                "租赁状态:正常\n剩余租期:{}"_trl(localeCode, time_utils::formatRemaining(land->getLeaseEndAt()));
            break;
        }
        case LeaseState::Frozen: {
            auto& priceService = PLand::getInstance().getServiceLocator().getLandPriceService();
            auto  detail       = priceService.calculateRenewCost(land, 0);
            leaseContent       = "租赁状态: 已冻结\n欠费: {}"_trl(localeCode, detail.total);
            break;
        }
        case LeaseState::Expired: {
            leaseContent = "租赁状态: 已到期(系统回收)"_trl(localeCode);
            break;
        }
        }
    }

    fm.setContent(
        "领地: {}\n类型: {}\n大小: {}x{}x{} = {}\n范围: {}\n{}\n{}"_trl(
            localeCode,
            land->getName(),
            land->is3D() ? "3D" : "2D",
            land->getAABB().getBlockCountX(),
            land->getAABB().getBlockCountZ(),
            land->getAABB().getBlockCountY(),
            land->getAABB().getVolume(),
            land->getAABB().toString(),
            leaseContent,
            subContent
        )
    );

    bool const isAdmin     = PLand::getInstance().getLandRegistry().isOperator(player.getUuid());
    bool const canOperLand = isAdmin ||             // 管理员
                             !land->isLeased() ||   // 未租赁(普通领地)
                             land->isLeaseActive(); // 租赁状态为正常

    if (canOperLand) {
        fm.appendButton("编辑权限"_trl(localeCode), "textures/ui/sidebar_icons/promotag", "path", [land](Player& pl) {
            sendEditLandPermGUI(pl, land);
        });
        fm.appendButton("修改成员"_trl(localeCode), "textures/ui/FriendsIcon", "path", [land](Player& pl) {
            sendChangeMember(pl, land);
        });
        fm.appendButton("修改领地名称"_trl(localeCode), "textures/ui/book_edit_default", "path", [land](Player& pl) {
            sendEditLandNameGUI(pl, land);
        });
    }

    if (land->isLeased()) {
        fm.appendButton("续费/缴费"_trl(localeCode), "textures/ui/MCoin", "path", [land](Player& pl) {
            sendLeaseRenewGUI(pl, land);
        });
    }

    if (canOperLand) {
        // 开启了领地传送功能，或者玩家是领地管理员
        if (ConfigProvider::isLandTeleportEnabled() || isAdmin) {
            fm.appendButton("传送到领地"_trl(localeCode), "textures/ui/icon_recipe_nature", "path", [land](Player& pl) {
                LandTeleportGUI::impl(pl, land);
            });

            // 如果玩家在领地内，则显示设置传送点按钮
            if (land->getAABB().hasPos(player.getPosition())) {
                fm.appendButton(
                    "设置传送点"_trl(localeCode),
                    "textures/ui/Add-Ons_Nav_Icon36x36",
                    "path",
                    [land](Player& pl) {
                        auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
                        if (auto res = service.setLandTeleportPos(pl, land, pl.getPosition())) {
                            feedback_utils::notifySuccess(pl, "传送点已设置!"_trl(pl.getLocaleCode()));
                        } else {
                            feedback_utils::sendError(pl, res.error());
                        }
                    }
                );
            }
        }

        fm.appendButton(
            "领地过户"_trl(localeCode),
            "textures/ui/sidebar_icons/my_characters",
            "path",
            [land](Player& pl) { sendTransferLandGUI(pl, land); }
        );

        if (ConfigProvider::isSubLandEnabled() && land->canCreateSubLand()) {
            fm.appendButton("创建子领地"_trl(localeCode), "textures/ui/icon_recipe_nature", "path", [land](Player& pl) {
                sendCreateSubLandConfirm(pl, land);
            });
        }

        if (land->isOrdinaryLand() && !land->isLeased()) {
            fm.appendButton("重新选区"_trl(localeCode), "textures/ui/anvil_icon", "path", [land](Player& pl) {
                sendChangeRangeConfirm(pl, land);
            });
        }

        fm.appendButton("删除领地"_trl(localeCode), "textures/ui/icon_trash", "path", [land](Player& pl) {
            showRemoveConfirm(pl, land);
        });
    }

    fm.sendTo(player);
}

void LandManagerGUI::sendLeaseRenewGUI(Player& player, std::shared_ptr<Land> const& land) {
    if (!land || !land->isLeased()) {
        return;
    }

    auto localeCode = player.getLocaleCode();

    std::string status;
    if (land->isLeaseFrozen()) {
        auto& priceService = PLand::getInstance().getServiceLocator().getLandPriceService();
        auto  detail       = priceService.calculateRenewCost(land, 0);
        status             = "当前状态: 已冻结\n欠费: {}"_trl(localeCode, detail.total);
    } else {
        status = "当前状态: 正常\n剩余到期时间: {}"_trl(localeCode, time_utils::formatRemaining(land->getLeaseEndAt()));
    }

    auto const& duration = ConfigProvider::getLeasingConfig().duration;

    CustomForm fm{"[PLand] | 租赁续费"_trl(localeCode)};
    fm.appendLabel(status);
    fm.appendSlider(
        "days",
        "续租天数"_trl(localeCode),
        duration.minPeriod,
        duration.maxPeriod,
        1.0,
        duration.minPeriod
    );
    fm.setSubmitButton("下一步"_trl(localeCode));
    fm.sendTo(player, [land](Player& pl, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        auto days = static_cast<int>(std::get<double>(res->at("days")));
        confirmRenewDuration(pl, land, days);
    });
}
void LandManagerGUI::confirmRenewDuration(Player& player, std::shared_ptr<Land> const& land, int days) {
    auto  localeCode   = player.getLocaleCode();
    auto& priceService = PLand::getInstance().getServiceLocator().getLandPriceService();
    auto  detail       = priceService.calculateRenewCost(land, days);

    std::string content = "续租天数: {}\n每日租金: {}\n欠费天数: {}\n滞纳金: {}\n总计: {}\n{}"_trl(
        localeCode,
        days,
        detail.dailyRent,
        detail.overdueDays,
        detail.penalty,
        detail.total,
        EconomySystem::getInstance().getCostMessage(player, detail.total)
    );

    auto confirm = SimpleForm{};
    confirm.setTitle("[PLand] | 确认续租"_trl(localeCode));
    confirm.setContent(content);
    confirm
        .appendButton("确认续租"_trl(localeCode), "textures/ui/realms_green_check", "path", [land, days](Player& pl2) {
            auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
            if (auto exp = service.renewLease(pl2, land, days)) {
                feedback_utils::notifySuccess(pl2, "续租成功"_trl(pl2.getLocaleCode()));
            } else {
                feedback_utils::sendError(pl2, exp.error());
            }
        });
    back_utils::injectBackButton(confirm, back_utils::wrapCallback<sendLeaseRenewGUI>(land));
    confirm.sendTo(player);
}
void LandManagerGUI::sendEditLandPermGUI(Player& player, std::shared_ptr<Land> const& ptr) {
    PermTableEditor::sendTo(
        player,
        ptr->getPermTable(),
        [ptr](Player& self, LandPermTable newTable) {
            ptr->setPermTable(newTable);
            feedback_utils::sendText(self, "权限表已更新"_trl(self.getLocaleCode()));
        },
        back_utils::wrapCallback<sendMainMenu>(ptr)
    );
}


void LandManagerGUI::showRemoveConfirm(Player& player, std::shared_ptr<Land> const& ptr) {
    switch (ptr->getType()) {
    case LandType::Ordinary:
    case LandType::Sub:
        confirmSimpleDelete(player, ptr);
        break;
    case LandType::Parent:
        confirmParentDelete(player, ptr);
        break;
    case LandType::Mix:
        confirmMixDelete(player, ptr);
        break;
    }
}

void LandManagerGUI::confirmSimpleDelete(Player& player, std::shared_ptr<Land> const& ptr) {
    if (!ptr->isOrdinaryLand() && !ptr->isSubLand()) {
        return;
    }
    auto localeCode = player.getLocaleCode();
    auto refund =
        ptr->isLeased() ? 0 : PLand::getInstance().getServiceLocator().getLandPriceService().getRefundAmount(ptr);
    auto content =
        ptr->isLeased()
            ? "您确定要删除领地 \"{}\" 吗?\n租赁领地删除后不会退还租金。\n此操作不可逆,请谨慎操作!"_trl(
                  localeCode,
                  ptr->getName()
              )
            : "您确定要删除领地 \"{}\" 吗?\n删除领地后，您将获得 {} 金币的退款。\n此操作不可逆,请谨慎操作!"_trl(
                  localeCode,
                  ptr->getName(),
                  refund
              );

    ModalForm("[PLand] | 确认删除?"_trl(localeCode), content, "确认"_trl(localeCode), "返回"_trl(localeCode))
        .sendTo(player, [ptr](Player& pl, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }
            if (!(bool)res.value()) {
                sendMainMenu(pl, ptr);
                return;
            }

            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
            if (auto expected = service.deleteLand(pl, ptr, service::DeletePolicy::CurrentOnly)) {
                feedback_utils::notifySuccess(pl, "删除成功"_trl(pl.getLocaleCode()));
            } else {
                feedback_utils::sendError(pl, expected.error());
            }
        });
}

void LandManagerGUI::confirmParentDelete(Player& player, std::shared_ptr<Land> const& ptr) {
    auto fm = SimpleForm{};
    gui::back_utils::injectBackButton<sendMainMenu>(fm, ptr);

    auto localeCode = player.getLocaleCode();
    fm.setTitle("[PLand] | 删除领地 & 父领地"_trl(localeCode));
    fm.setContent(
        "您当前操作的的是父领地\n当前领地下有 {} 个子领地\n您确定要删除领地吗?"_trl(
            localeCode,
            ptr->getSubLandIDs().size()
        )
    );

    fm.appendButton("删除当前领地和子领地"_trl(localeCode), [ptr](Player& pl) {
        auto expected = PLand::getInstance().getServiceLocator().getLandManagementService().deleteLand(
            pl,
            ptr,
            service::DeletePolicy::Recursive
        );
        if (expected) {
            feedback_utils::notifySuccess(pl, "删除成功"_trl(pl.getLocaleCode()));
        } else {
            feedback_utils::sendError(pl, expected.error());
        }
    });
    fm.appendButton("删除当前领地并提升子领地为父领地"_trl(localeCode), [ptr](Player& pl) {
        auto expected = PLand::getInstance().getServiceLocator().getLandManagementService().deleteLand(
            pl,
            ptr,
            service::DeletePolicy::PromoteChildren
        );
        if (expected) {
            feedback_utils::notifySuccess(pl, "删除成功"_trl(pl.getLocaleCode()));
        } else {
            feedback_utils::sendError(pl, expected.error());
        }
    });

    fm.sendTo(player);
}

void LandManagerGUI::confirmMixDelete(Player& player, std::shared_ptr<Land> const& ptr) {
    auto localeCode = player.getLocaleCode();

    auto fm = SimpleForm{};
    gui::back_utils::injectBackButton<sendMainMenu>(fm, ptr);

    fm.setTitle("[PLand] | 删除领地 & 混合领地"_trl(localeCode));
    fm.setContent(
        "您当前操作的的是混合领地\n当前领地下有 {} 个子领地\n您确定要删除领地吗?"_trl(
            localeCode,
            ptr->getSubLandIDs().size()
        )
    );

    fm.appendButton("删除当前领地和子领地"_trl(localeCode), [ptr](Player& pl) {
        auto expected = PLand::getInstance().getServiceLocator().getLandManagementService().deleteLand(
            pl,
            ptr,
            service::DeletePolicy::Recursive
        );
        if (expected) {
            feedback_utils::notifySuccess(pl, "删除成功"_trl(pl.getLocaleCode()));
        } else {
            feedback_utils::sendError(pl, expected.error());
        }
    });
    fm.appendButton("删除当前领地并移交子领地给父领地"_trl(localeCode), [ptr](Player& pl) {
        auto expected = PLand::getInstance().getServiceLocator().getLandManagementService().deleteLand(
            pl,
            ptr,
            service::DeletePolicy::TransferChildren
        );
        if (expected) {
            feedback_utils::notifySuccess(pl, "删除成功"_trl(pl.getLocaleCode()));
        } else {
            feedback_utils::sendError(pl, expected.error());
        }
    });

    fm.sendTo(player);
}

void LandManagerGUI::sendEditLandNameGUI(Player& player, std::shared_ptr<Land> const& ptr) {
    auto localeCode = player.getLocaleCode();

    gui::SimpleInputForm::sendTo(
        player,
        "修改领地名称"_trl(localeCode),
        "请输入新的领地名称"_trl(localeCode),
        ptr->getName(),
        [ptr](Player& pl, std::string result) {
            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
            if (auto expected = service.setLandName(pl, ptr, result)) {
                feedback_utils::sendText(pl, "领地名称已更新!"_trl(pl.getLocaleCode(), result));
            } else {
                feedback_utils::sendError(pl, expected.error());
            }
        }
    );
}
void LandManagerGUI::sendTransferLandGUI(Player& player, std::shared_ptr<Land> const& ptr) {
    auto localeCode = player.getLocaleCode();

    auto fm = SimpleForm{};
    gui::back_utils::injectBackButton<sendMainMenu>(fm, ptr);

    fm.setTitle("[PLand] | 转让领地"_trl(localeCode));
    fm.appendButton(
        "转让给在线玩家"_trl(localeCode),
        "textures/ui/sidebar_icons/my_characters",
        "path",
        [ptr](Player& self) { _sendTransferLandToOnlinePlayer(self, ptr); }
    );

    fm.appendButton(
        "转让给离线玩家"_trl(localeCode),
        "textures/ui/sidebar_icons/my_characters",
        "path",
        [ptr](Player& self) { _sendTransferLandToOfflinePlayer(self, ptr); }
    );

    fm.sendTo(player);
}
void LandManagerGUI::_sendTransferLandToOnlinePlayer(Player& player, const std::shared_ptr<Land>& ptr) {
    gui::OnlinePlayerPicker::sendTo(
        player,
        [ptr](Player& self, Player& target) {
            _confirmTransferLand(self, ptr, target.getUuid(), target.getRealName());
        },
        gui::back_utils::wrapCallback<sendTransferLandGUI>(ptr)
    );
}
void LandManagerGUI::_sendTransferLandToOfflinePlayer(Player& player, std::shared_ptr<Land> const& ptr) {
    auto localeCode = player.getLocaleCode();

    CustomForm fm("[PLand] | 转让给离线玩家"_trl(localeCode));
    fm.appendInput("playerName", "请输入离线玩家名称"_trl(localeCode), "玩家名称");
    fm.sendTo(player, [ptr](Player& self, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        auto localeCode = self.getLocaleCode();

        auto playerName = std::get<std::string>(res->at("playerName"));
        if (playerName.empty()) {
            feedback_utils::sendErrorText(self, "玩家名称不能为空!"_trl(localeCode));
            sendTransferLandGUI(self, ptr);
            return;
        }

        auto playerInfo = ll::service::PlayerInfo::getInstance().fromName(playerName);
        if (!playerInfo) {
            feedback_utils::sendErrorText(self, "未找到该玩家信息，请检查名称是否正确!"_trl(localeCode));
            sendTransferLandGUI(self, ptr);
            return;
        }
        auto& targetUuid = playerInfo->uuid;
        _confirmTransferLand(self, ptr, targetUuid, playerName);
    });
}
void LandManagerGUI::_confirmTransferLand(
    Player&                      player,
    const std::shared_ptr<Land>& ptr,
    mce::UUID                    target,
    std::string                  displayName
) {
    auto localeCode = player.getLocaleCode();

    ModalForm(
        "[PLand] | 确认转让"_trl(localeCode),
        "您确定要将领地转让给 {} 吗?\n转让后，您将失去此领地的权限。\n此操作不可逆,请谨慎操作!"_trl(
            localeCode,
            displayName
        ),
        "确认"_trl(localeCode),
        "返回"_trl(localeCode)
    )
        .sendTo(player, [ptr, target, displayName](Player& player, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }
            if (!(bool)res.value()) {
                sendTransferLandGUI(player, ptr);
                return;
            }
            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
            if (auto expected = service.transferLand(player, ptr, target)) {
                auto localeCode = player.getLocaleCode();

                feedback_utils::sendText(player, "领地已转让给 {}"_trl(localeCode, displayName));
                if (auto targetPlayer = player.getLevel().getPlayer(target)) {
                    feedback_utils::sendText(
                        *targetPlayer,
                        "您已接手来自 \"{}\" 的领地 \"{}\""_trl(localeCode, player.getRealName(), ptr->getName())
                    );
                }
            } else {
                feedback_utils::sendError(player, expected.error());
            }
        });
}

void LandManagerGUI::sendCreateSubLandConfirm(Player& player, const std::shared_ptr<Land>& ptr) {
    auto localeCode = player.getLocaleCode();

    ModalForm{
        "[PLand] | 子领地创建确认"_trl(localeCode),
        "将在当前领地内划出一个新的子领地\n\n子领地将成为独立领地：\n• 位于当前领地范围内\n• 权限完全独立（类似租界）\n是否继续？"_trl(
            localeCode
        ),
        "创建子领地"_trl(localeCode),
        "返回"_trl(localeCode)
    }
        .sendTo(player, [ptr](Player& player, ModalFormResult const& res, FormCancelReason) {
            if (!res) return;
            if (!(bool)res.value()) {
                LandManagerGUI::sendMainMenu(player, ptr);
                return;
            }
            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
            if (auto expected = service.requestCreateSubLand(player)) {
                feedback_utils::sendText(
                    player,
                    "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trl(
                        player.getLocaleCode(),
                        ConfigProvider::getSelectionConfig().alias
                    )
                );
            } else {
                feedback_utils::sendError(player, expected.error());
            }
        });
}

void LandManagerGUI::sendChangeRangeConfirm(Player& player, std::shared_ptr<Land> const& ptr) {
    auto localeCode = player.getLocaleCode();

    ModalForm fm(
        "[PLand] | 重新选区"_trl(localeCode),
        "重新选区为完全重新选择领地的范围，非直接扩充/缩小现有领地范围。\n重新选择的价格计算方式为\"新范围价格 — 旧范围价值\"，是否继续？"_trl(
            localeCode
        ),
        "确认"_trl(localeCode),
        "返回"_trl(localeCode)
    );
    fm.sendTo(player, [ptr](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        if (!(bool)res.value()) {
            LandManagerGUI::sendMainMenu(self, ptr);
            return;
        }
        auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
        if (auto expected = service.requestChangeRange(self, ptr)) {
            feedback_utils::sendText(
                self,
                "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trl(
                    self.getLocaleCode(),
                    ConfigProvider::getSelectionConfig().alias
                )
            );
        } else {
            feedback_utils::sendError(self, expected.error());
        }
    });
}


void LandManagerGUI::sendChangeMember(Player& player, std::shared_ptr<Land> ptr) {
    auto fm = SimpleForm{};
    gui::back_utils::injectBackButton<sendMainMenu>(fm, ptr);

    auto localeCode = player.getLocaleCode();
    fm.appendButton("添加在线成员"_trl(localeCode), "textures/ui/color_plus", "path", [ptr](Player& self) {
        _sendAddOnlineMember(self, ptr);
    });
    fm.appendButton("添加离线成员"_trl(localeCode), "textures/ui/color_plus", "path", [ptr](Player& self) {
        _sendAddOfflineMember(self, ptr);
    });

    auto& infos = ll::service::PlayerInfo::getInstance();
    for (auto& member : ptr->getMembers()) {
        auto i = infos.fromUuid(member);
        fm.appendButton(i.has_value() ? i->name : member.asString(), [member, ptr](Player& self) {
            _confirmRemoveMember(self, ptr, member);
        });
    }

    fm.sendTo(player);
}
void LandManagerGUI::_sendAddOnlineMember(Player& player, std::shared_ptr<Land> ptr) {
    gui::OnlinePlayerPicker::sendTo(
        player,
        [ptr](Player& self, Player& target) { _confirmAddMember(self, ptr, target.getUuid(), target.getRealName()); },
        gui::back_utils::wrapCallback<sendChangeMember>(ptr)
    );
}
void LandManagerGUI::_sendAddOfflineMember(Player& player, std::shared_ptr<Land> ptr) {
    auto localeCode = player.getLocaleCode();

    CustomForm fm("[PLand] | 添加离线成员"_trl(localeCode));
    fm.appendInput("playerName", "请输入离线玩家名称"_trl(localeCode), "玩家名称");
    fm.sendTo(player, [ptr](Player& self, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        auto localeCode = self.getLocaleCode();

        auto playerName = std::get<std::string>(res->at("playerName"));
        if (playerName.empty()) {
            feedback_utils::sendErrorText(self, "玩家名称不能为空!"_trl(localeCode));
            sendChangeMember(self, ptr);
            return;
        }

        auto playerInfo = ll::service::PlayerInfo::getInstance().fromName(playerName);
        if (!playerInfo) {
            feedback_utils::sendErrorText(self, "未找到该玩家信息，请检查名称是否正确!"_trl(localeCode));
            sendChangeMember(self, ptr);
            return;
        }

        auto& targetUuid = playerInfo->uuid;
        _confirmAddMember(self, ptr, targetUuid, playerName);
    });
}
void LandManagerGUI::_confirmAddMember(
    Player&               player,
    std::shared_ptr<Land> ptr,
    mce::UUID             member,
    std::string           displayName
) {
    auto localeCode = player.getLocaleCode();

    ModalForm fm(
        "[PLand] | 添加成员"_trl(localeCode),
        "您确定要添加 {} 为领地成员吗?"_trl(localeCode, displayName),
        "确认"_trl(localeCode),
        "返回"_trl(localeCode)
    );
    fm.sendTo(player, [ptr, member](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        if (!(bool)res.value()) {
            sendChangeMember(self, ptr);
            return;
        }

        auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
        if (auto expected = service.addMember(self, ptr, member)) {
            feedback_utils::sendText(self, "添加成功!"_trl(self.getLocaleCode()));
        } else {
            feedback_utils::sendError(self, expected.error());
        }
    });
}
void LandManagerGUI::_confirmRemoveMember(Player& player, std::shared_ptr<Land> ptr, mce::UUID member) {
    auto info       = ll::service::PlayerInfo::getInstance().fromUuid(member);
    auto localeCode = player.getLocaleCode();

    ModalForm fm(
        "[PLand] | 移除成员"_trl(localeCode),
        "您确定要移除成员 \"{}\" 吗?"_trl(localeCode, info.has_value() ? info->name : member.asString()),
        "确认"_trl(localeCode),
        "返回"_trl(localeCode)
    );
    fm.sendTo(player, [ptr, member](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        if (!(bool)res.value()) {
            sendChangeMember(self, ptr);
            return;
        }

        auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
        if (auto expected = service.removeMember(self, ptr, member)) {
            feedback_utils::sendText(self, "移除成员成功!"_trl(self.getLocaleCode()));
        } else {
            feedback_utils::sendError(self, expected.error());
        }
    });
}


} // namespace land::gui
