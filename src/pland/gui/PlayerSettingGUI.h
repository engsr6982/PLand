#pragma once
#include "pland/Global.h"

class Player;

namespace land {


class PlayerSettingGUI {
public:
    PlayerSettingGUI() = delete;

    LDAPI static void sendTo(Player& player);
};


} // namespace land