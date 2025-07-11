#pragma once
#include "pland/Global.h"
#include "pland/land/Land.h"


namespace land {


class LandManagerGUI {
public:
    LandManagerGUI() = delete;

    LDAPI static void sendMainMenu(Player& player, SharedLand land);

    LDAPI static void sendEditLandPermGUI(Player& player, SharedLand const& ptr); // 编辑领地权限

    // 删除领地
    LDAPI static void sendRemoveConfirmGUI(Player& player, SharedLand const& ptr);                // 删除领地确认
    LDAPI static void _implRemoveWithOrdinaryOrSubLandGUI(Player& player, SharedLand const& ptr); // 删除普通或子领地
    LDAPI static void _implRemoveParentLandGUI(Player& player, SharedLand const& ptr);            // 删除父领地
    LDAPI static void _implRemoveMixLandGUI(Player& player, SharedLand const& ptr);               // 删除混合领地

    LDAPI static void sendEditLandNameGUI(Player& player, SharedLand const& ptr);   // 编辑领地名称
    LDAPI static void sendEditLandDescGUI(Player& player, SharedLand const& ptr);   // 编辑领地描述
    LDAPI static void sendTransferLandGUI(Player& player, SharedLand const& ptr);   // 转让领地
    LDAPI static void sendChangLandRangeGUI(Player& player, SharedLand const& ptr); // 更改领地范围

    LDAPI static void sendChangeMemberGUI(Player& player, SharedLand ptr);                 // 更改成员
    LDAPI static void _sendAddMemberGUI(Player& player, SharedLand ptr);                   // 添加成员
    LDAPI static void _sendRemoveMemberGUI(Player& player, SharedLand ptr, UUIDs members); // 移除成员
};


} // namespace land