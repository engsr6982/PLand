#pragma once
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/gui/form/BackSimpleForm.h"


class Player;

namespace land {

class ChooseLandUtilGUI {
public:
    ChooseLandUtilGUI() = delete;

    using ChooseCallback = std::function<void(Player&, LandID id)>;
    LDAPI static void sendTo(
        Player&                          player,
        ChooseCallback const&            callback,
        bool                             showShredLand = false,
        BackSimpleForm<>::ButtonCallback back          = {}
    );
};


class ChoosePlayerUtilGUI {
public:
    ChoosePlayerUtilGUI() = delete;

    using ChoosePlayerCall = std::function<void(Player&, Player* choosedPlayer)>;
    LDAPI static void
    sendTo(Player& player, ChoosePlayerCall const& callback, BackSimpleForm<>::ButtonCallback back = {});
};


class EditStringUtilGUI {
public:
    EditStringUtilGUI() = delete;

    using EditStringResult = std::function<void(Player& self, string result)>;
    LDAPI static void sendTo(
        Player&          player,
        string const&    title,       // 标题
        string const&    text,        // 提示
        string const&    defaultValu, // 默认值
        EditStringResult callback     // 回调
    );
};


class FuzzySerarchUtilGUI {
public:
    FuzzySerarchUtilGUI() = delete;

    using CallBack = std::function<void(Player& slef, std::vector<LandData_sptr> result)>;
    LDAPI static void sendTo(Player& player, std::vector<LandData_sptr> list, CallBack callback);
};


} // namespace land