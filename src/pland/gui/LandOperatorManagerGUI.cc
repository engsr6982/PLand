#include "LandOperatorManagerGUI.h"
#include "CommonUtilGUI.h"
#include "LandManagerGUI.h"
#include "ll/api/service/PlayerInfo.h"
#include "pland/PLand.h"
#include "pland/gui/form/BackPaginatedSimpleForm.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/utils/McUtils.h"


namespace land {


void LandOperatorManagerGUI::sendMainMenu(Player& player) {
    auto* db = &PLand::getInstance();
    if (!db->isOperator(player.getUuid().asString())) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "无权限访问此表单"_trf(player));
        return;
    }

    auto fm = BackSimpleForm<>::make();

    fm.setTitle(PLUGIN_NAME + " | 领地管理"_trf(player));
    fm.setContent("请选择您要进行的操作"_trf(player));

    fm.appendButton("管理脚下领地"_trf(player), "textures/ui/free_download", "path", [db](Player& self) {
        auto lands = db->getLandAt(self.getPosition(), self.getDimensionId());
        if (!lands) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "您当前所处位置没有领地"_trf(self));
            return;
        }
        LandManagerGUI::impl(self, lands->getLandID());
    });
    fm.appendButton("管理玩家领地"_trf(player), "textures/ui/FriendsIcon", "path", [](Player& self) {
        sendChoosePlayerFromDb(self, static_cast<void (*)(Player&, UUIDs const&)>(sendChooseLandGUI));
    });
    fm.appendButton("管理指定领地"_trf(player), "textures/ui/magnifyingGlass", "path", [](Player& self) {
        sendChooseLandGUI(self, PLand::getInstance().getLands());
    });

    fm.sendTo(player);
}


void LandOperatorManagerGUI::sendChoosePlayerFromDb(Player& player, ChoosePlayerCallback callback) {
    auto fm = BackSimpleForm<>::make<LandOperatorManagerGUI::sendMainMenu>();
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


void LandOperatorManagerGUI::sendChooseLandGUI(Player& player, UUIDs const& targetPlayer) {
    sendChooseLandGUI(player, PLand::getInstance().getLands(targetPlayer));
}

void LandOperatorManagerGUI::sendChooseLandGUI(Player& player, std::vector<LandData_sptr> lands) {
    // auto fm = BackSimpleForm<>::make<LandOperatorManagerGUI::sendMainMenu>();
    auto fm = BackSimpleForm<BackPaginatedSimpleForm>::make<LandOperatorManagerGUI::sendMainMenu>();
    fm.setTitle(PLUGIN_NAME + " | 领地列表"_trf(player));
    fm.setContent("请选择您要管理的领地"_trf(player));

    fm.appendButton("模糊搜索领地", "textures/ui/magnifyingGlass", "path", [lands](Player& player) {
        FuzzySerarchUtilGUI::sendTo(
            player,
            lands,
            static_cast<void (*)(Player&, std::vector<std::shared_ptr<LandData>>)>(sendChooseLandGUI)
        );
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
            [ptr](Player& self) { LandManagerGUI::impl(self, ptr->getLandID()); }
        );
    }

    fm.sendTo(player);
}


} // namespace land