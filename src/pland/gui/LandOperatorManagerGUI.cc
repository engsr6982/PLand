#include "LandOperatorManagerGUI.h"
#include "CommonUtilGUI.h"
#include "LandManagerGUI.h"
#include "ll/api/service/PlayerInfo.h"
#include "pland/PLand.h"
#include "pland/gui/common/ChooseLandAdvancedUtilGUI.h"
#include "pland/gui/common/EditLandPermTableUtilGUI.h"
#include "pland/gui/form/BackPaginatedSimpleForm.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/land/LandContext.h"
#include "pland/land/LandRegistry.h"
#include "pland/land/LandTemplatePermTable.h"
#include "pland/utils/McUtils.h"


namespace land {


void LandOperatorManagerGUI::sendMainMenu(Player& player) {
    if (!PLand::getInstance().getLandRegistry()->isOperator(player.getUuid().asString())) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "无权限访问此表单"_trf(player));
        return;
    }

    auto fm = BackSimpleForm<>::make();

    fm.setTitle(PLUGIN_NAME + " | 领地管理"_trf(player));
    fm.setContent("请选择您要进行的操作"_trf(player));

    fm.appendButton("管理脚下领地"_trf(player), "textures/ui/free_download", "path", [](Player& self) {
        auto lands = PLand::getInstance().getLandRegistry()->getLandAt(self.getPosition(), self.getDimensionId());
        if (!lands) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "您当前所处位置没有领地"_trf(self));
            return;
        }
        LandManagerGUI::sendMainMenu(self, lands);
    });
    fm.appendButton("管理玩家领地"_trf(player), "textures/ui/FriendsIcon", "path", [](Player& self) {
        sendChoosePlayerFromDb(self, static_cast<void (*)(Player&, UUIDs const&)>(sendChooseLandGUI));
    });
    fm.appendButton("管理指定领地"_trf(player), "textures/ui/magnifyingGlass", "path", [](Player& self) {
        // sendChooseLandGUI(self, PLand::getInstance().getLandRegistry()->getLands());
        sendChooseLandAdvancedGUI(self, PLand::getInstance().getLandRegistry()->getLands());
    });
    fm.appendButton("编辑默认权限"_trf(player), "textures/ui/icon_map", "path", [](Player& self) {
        EditLandPermTableUtilGUI::sendTo(
            self,
            PLand::getInstance().getLandRegistry()->getLandTemplatePermTable().get(),
            [](Player& self, LandPermTable newTable) {
                PLand::getInstance().getLandRegistry()->getLandTemplatePermTable().set(newTable);
                mc_utils::sendText(self, "权限表已更新"_trf(self));
            }
        );
    });

    fm.sendTo(player);
}


void LandOperatorManagerGUI::sendChoosePlayerFromDb(Player& player, ChoosePlayerCallback callback) {
    auto fm = BackSimpleForm<>::make<LandOperatorManagerGUI::sendMainMenu>();
    fm.setTitle(PLUGIN_NAME + " | 玩家列表"_trf(player));
    fm.setContent("请选择您要管理的玩家"_trf(player));

    auto const& infos = ll::service::PlayerInfo::getInstance();
    auto const  lands = PLand::getInstance().getLandRegistry()->getLands();

    std::unordered_set<UUIDs> filtered; // 防止重复
    for (auto const& ptr : lands) {
        if (filtered.contains(ptr->getOwner())) {
            continue;
        }
        filtered.insert(ptr->getOwner());
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getOwner()));

        fm.appendButton(info.has_value() ? info->name : ptr->getOwner(), [ptr, callback](Player& self) {
            callback(self, ptr->getOwner());
        });
    }

    fm.sendTo(player);
}


void LandOperatorManagerGUI::sendChooseLandGUI(Player& player, UUIDs const& targetPlayer) {
    // sendChooseLandGUI(player, PLand::getInstance().getLandRegistry()->getLands(targetPlayer));
    sendChooseLandAdvancedGUI(player, PLand::getInstance().getLandRegistry()->getLands(targetPlayer));
}

void LandOperatorManagerGUI::sendChooseLandAdvancedGUI(Player& player, std::vector<SharedLand> lands) {
    ChooseLandAdvancedUtilGUI::sendTo(
        player,
        lands,
        [](Player& self, SharedLand ptr) { LandManagerGUI::sendMainMenu(self, ptr); },
        BackSimpleForm<>::makeCallback<sendMainMenu>()
    );
}

void LandOperatorManagerGUI::sendChooseLandGUI(Player& player, std::vector<SharedLand> lands) {
    // auto fm = BackSimpleForm<>::make<LandOperatorManagerGUI::sendMainMenu>();
    auto fm = BackSimpleForm<BackPaginatedSimpleForm>::make<LandOperatorManagerGUI::sendMainMenu>();
    fm.setTitle(PLUGIN_NAME + " | 领地列表"_trf(player));
    fm.setContent("请选择您要管理的领地"_trf(player));

    fm.appendButton("模糊搜索领地", "textures/ui/magnifyingGlass", "path", [lands](Player& player) {
        FuzzySerarchUtilGUI::sendTo(
            player,
            lands,
            static_cast<void (*)(Player&, std::vector<std::shared_ptr<Land>>)>(sendChooseLandGUI)
        );
    });

    auto const& infos = ll::service::PlayerInfo::getInstance();
    for (auto const& ptr : lands) {
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getOwner()));
        fm.appendButton(
            "{}\nID: {}  玩家: {}"_trf(
                player,
                ptr->getName(),
                ptr->getId(),
                info.has_value() ? info->name : ptr->getOwner()
            ),
            [ptr](Player& self) { LandManagerGUI::sendMainMenu(self, ptr); }
        );
    }

    fm.sendTo(player);
}


} // namespace land