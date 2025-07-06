#pragma once
#include "pland/Global.h"

class Player;

namespace land {


class LandMainMenuGUI {
public:
    LandMainMenuGUI() = delete;

    LDAPI static void sendTo(Player& player);
};


} // namespace land