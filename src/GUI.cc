#include "pland/GUI.h"
#include "fmt/compile.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/PlayerInfo.h"
#include "mc/world/actor/player/Player.h"
#include "mod/ModEntry.h"
#include "pland/Config.h"
#include "pland/DrawHandleManager.h"
#include "pland/EconomySystem.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandEvent.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/PriceCalculate.h"
#include "pland/SafeTeleport.h"
#include "pland/gui/CommonUtilGui.hpp"
#include "pland/gui/LandManageGui.h"
#include "pland/math/LandAABB.h"
#include "pland/utils/JSON.h"
#include "pland/utils/McUtils.h"
#include "pland/utils/Utils.h"
#include "pland/wrapper/FormEx.h"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>


using namespace ll::form;


namespace land {

// 独立GUI
void ChooseLandDimAndNewLand::impl(Player& player) {
    if (!some(Config::cfg.land.bought.allowDimensions, player.getDimensionId().id)) {
        mc_utils::sendText(player, "你所在的维度无法购买领地"_trf(player));
        return;
    }

    PlayerAskCreateLandBeforeEvent ev(player);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    ModalForm(
        PLUGIN_NAME + ("| 选择领地维度"_trf(player)),
        "请选择领地维度\n\n2D: 领地拥有整个Y轴\n3D: 自行选择Y轴范围"_trf(player),
        "2D", // true
        "3D"  // false
    )
        .sendTo(player, [](Player& pl, ModalFormResult res, FormCancelReason) {
            if (!res.has_value()) {
                return;
            }

            bool land3D = !((bool)res.value());
            if (land3D && !Config::cfg.land.bought.threeDimensionl.enabled) {
                mc_utils::sendText(pl, "3D领地功能未启用，请联系管理员"_trf(pl));
                return;
            }
            if (!land3D && !Config::cfg.land.bought.twoDimensionl.enabled) {
                mc_utils::sendText(pl, "2D领地功能未启用，请联系管理员"_trf(pl));
                return;
            }

            PlayerAskCreateLandAfterEvent ev(pl, land3D);
            ll::event::EventBus::getInstance().publish(ev);

            if (SelectorManager::getInstance().start(Selector::createDefault(pl, pl.getDimensionId().id, land3D))) {
                mc_utils::sendText(
                    pl,
                    "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trf(pl, Config::cfg.selector.tool)
                );
            } else {
                mc_utils::sendText(pl, "选区开启失败，当前存在未完成的选区任务"_trf(pl));
            }
        });
}
void SelectorChangeYGui::impl(Player& player, std::string const& exception) {
    auto selector = SelectorManager::getInstance().get(player);
    if (!selector) {
        return;
    }
    selector->fixAABBMinMax();

    CustomForm fm(PLUGIN_NAME + ("| 确认Y轴范围"_trf(player)));

    fm.appendLabel("确认选区的Y轴范围\n您可以在此调节Y轴范围，如果不需要修改，请直接点击提交"_trf(player));

    bool const       isSubLand      = selector->getType() == Selector::Type::SubLand;
    SubLandSelector* subSelector    = nullptr;
    LandData_sptr    parentLandData = nullptr;
    if (isSubLand) {
        if (subSelector = selector->As<SubLandSelector>(); subSelector) {
            if (parentLandData = subSelector->getParentLandData(); parentLandData) {
                auto& aabb = parentLandData->getLandPos();
                fm.appendLabel("当前为子领地模式，子领地的Y轴范围不能超过父领地。\n父领地Y轴范围: {} ~ {}"_trf(
                    player,
                    aabb.min.y,
                    aabb.max.y
                ));
            }
        }
    }

    fm.appendInput("start", "开始Y轴"_trf(player), "int", std::to_string(selector->getPointA()->y));
    fm.appendInput("end", "结束Y轴"_trf(player), "int", std::to_string(selector->getPointB()->y));

    fm.appendLabel(exception);

    fm.sendTo(
        player,
        [selector, isSubLand, subSelector, parentLandData](Player& pl, CustomFormResult res, FormCancelReason) {
            if (!res.has_value()) {
                return;
            }

            string start = std::get<string>(res->at("start"));
            string end   = std::get<string>(res->at("end"));

            if (!isNumber(start) || !isNumber(end) || isOutOfRange(start) || isOutOfRange(end)) {
                SelectorChangeYGui::impl(pl, "请输入正确的Y轴范围"_trf(pl));
                return;
            }

            try {
                int startY = std::stoi(start);
                int endY   = std::stoi(end);

                if (startY >= endY) {
                    SelectorChangeYGui::impl(pl, "请输入正确的Y轴范围, 开始Y轴必须小于结束Y轴"_trf(pl));
                    return;
                }

                if (isSubLand) {
                    if (!subSelector || !parentLandData) {
                        throw std::runtime_error("subSelector or parentLandData is nullptr");
                    }

                    auto& aabb = parentLandData->getLandPos();
                    if (startY < aabb.min.y || endY > aabb.max.y) {
                        SelectorChangeYGui::impl(pl, "请输入正确的Y轴范围, 子领地的Y轴范围不能超过父领地"_trf(pl));
                        return;
                    }
                }

                selector->mPointA->y = startY;
                selector->mPointB->y = endY;
                selector->fixAABBMinMax();
                selector->onFixesY();
                mc_utils::sendText(pl, "Y轴范围已修改为 {} ~ {}"_trf(pl, startY, endY));
            } catch (...) {
                mc_utils::sendText<mc_utils::LogLevel::Fatal>(pl, "插件内部错误, 请联系开发者"_trf(pl));
                mod::ModEntry::getInstance().getSelf().getLogger().error(
                    "An exception is caught in {} and user {} enters data: {}, {}",
                    __FUNCTION__,
                    pl.getRealName(),
                    start,
                    end
                );
            }
        }
    );
}


// 领地主菜单
void LandMainGui::impl(Player& player) {
    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 领地菜单"_trf(player)));
    fm.setContent("欢迎使用 Pland 领地管理插件\n\n请选择一个功能"_trf(player));

    fm.appendButton("新建领地"_trf(player), "textures/ui/anvil_icon", [](Player& pl) {
        ChooseLandDimAndNewLand::impl(pl);
    });
    fm.appendButton("管理领地"_trf(player), "textures/ui/icon_spring", [](Player& pl) {
        ChooseLandUtilGui::impl<LandMainGui>(pl, LandManageGui::impl);
    });

    if (Config::cfg.land.landTp || PLand::getInstance().isOperator(player.getUuid().asString())) {
        fm.appendButton("领地传送"_trf(player), "textures/ui/icon_recipe_nature", [](Player& pl) {
            LandTeleportGui::impl(pl);
        });
    }

    fm.appendButton("个人设置"_trf(player), "textures/ui/icon_recipe_nature", [](Player& pl) {
        EditPlayerSettingGui::impl(pl);
    });

    fm.appendButton("关闭"_trf(player), "textures/ui/cancel");
    fm.sendTo(player);
}


void EditPlayerSettingGui::impl(Player& player) {
    auto setting = PLand::getInstance().getPlayerSettings(player.getUuid().asString());

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


// 领地管理菜单


// 编辑领地成员
void EditLandMemberGui::impl(Player& player, LandData_sptr ptr) {
    auto fm = SimpleFormEx::create<LandManageGui, BackButtonPos::Upper>(ptr->getLandID());

    fm.appendButton("添加成员"_trf(player), "textures/ui/color_plus", [ptr](Player& self) {
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
void EditLandMemberGui::AddMemberGui::impl(Player& player, LandData_sptr ptr) {
    ChoosePlayerUtilGui::impl<EditLandMemberGui>(player, [ptr](Player& self, Player* target) {
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
    });
}
void EditLandMemberGui::RemoveMemberGui::impl(Player& player, LandData_sptr ptr, UUIDs member) {
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


// 领地传送GUI
void LandTeleportGui::impl(Player& player) { ChooseLandUtilGui::impl<LandMainGui>(player, LandTeleportGui::run, true); }
void LandTeleportGui::run(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地不存在"_trf(player));
        return;
    }

    if (land->mTeleportPos.isZero()) {
        SafeTeleport::getInstance().teleportTo(player, land->mPos.min, land->getLandDimid());
        return;
    }

    player.teleport(land->mTeleportPos, land->getLandDimid());
}


// 管理员GUI
void LandOPManagerGui::impl(Player& player) {
    auto* db = &PLand::getInstance();
    if (!db->isOperator(player.getUuid().asString())) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "无权限访问此表单"_trf(player));
        return;
    }

    auto fm = SimpleFormEx::create();

    fm.setTitle(PLUGIN_NAME + " | 领地管理"_trf(player));
    fm.setContent("请选择您要进行的操作"_trf(player));

    fm.appendButton("管理脚下领地"_trf(player), "textures/ui/free_download", [db](Player& self) {
        auto lands = db->getLandAt(self.getPosition(), self.getDimensionId());
        if (!lands) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "您当前所处位置没有领地"_trf(self));
            return;
        }
        LandManageGui::impl(self, lands->getLandID());
    });
    fm.appendButton("管理玩家领地"_trf(player), "textures/ui/FriendsIcon", [](Player& self) {
        IChoosePlayerFromDB::impl(self, ManageLandWithPlayer::impl);
    });
    fm.appendButton("管理指定领地"_trf(player), "textures/ui/magnifyingGlass", [](Player& self) {
        IChooseLand::impl(self, PLand::getInstance().getLands());
    });

    fm.sendTo(player);
}
void LandOPManagerGui::ManageLandWithPlayer::impl(Player& player, UUIDs const& targetPlayer) {
    IChooseLand::impl(player, PLand::getInstance().getLands(targetPlayer));
}


void LandOPManagerGui::IChoosePlayerFromDB::impl(Player& player, ChoosePlayerCall callback) {
    auto fm = SimpleFormEx::create<LandOPManagerGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + " | 玩家列表"_trf(player));
    fm.setContent("请选择您要管理的玩家"_trf(player));

    auto const& db    = PLand::getInstance();
    auto const& infos = ll::service::PlayerInfo::getInstance();
    auto const  lands = db.getLands();

    std::unordered_set<UUIDs> filtered; // 防止重复
    for (auto const& ptr : lands) {
        if (filtered.contains(ptr->getLandOwner())) {
            continue;
        }
        filtered.insert(ptr->getLandOwner());
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getLandOwner()));

        fm.appendButton(info.has_value() ? info->name : ptr->getLandOwner(), [ptr, callback](Player& self) {
            callback(self, ptr->getLandOwner());
        });
    }

    fm.sendTo(player);
}


void LandOPManagerGui::IChooseLand::impl(Player& player, std::vector<LandData_sptr> const& lands) {
    auto fm = SimpleFormEx::create<LandOPManagerGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + " | 领地列表"_trf(player));
    fm.setContent("请选择您要管理的领地"_trf(player));

    fm.appendButton("模糊搜索领地", "textures/ui/magnifyingGlass", [lands](Player& player) {
        FuzzySerarchUtilGui::impl(player, lands, LandOPManagerGui::IChooseLand::impl);
    });

    auto const& infos = ll::service::PlayerInfo::getInstance();
    for (auto const& ptr : lands) {
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getLandOwner()));
        fm.appendButton(
            "{}\nID: {}  玩家: {}"_trf(
                player,
                ptr->getLandName(),
                ptr->getLandID(),
                info.has_value() ? info->name : ptr->getLandOwner()
            ),
            [ptr](Player& self) { LandManageGui::impl(self, ptr->getLandID()); }
        );
    }

    fm.sendTo(player);
}


} // namespace land
