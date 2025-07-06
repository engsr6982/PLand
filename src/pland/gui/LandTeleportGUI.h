#pragma once
#include "pland/Global.h"

class Player;

namespace land {


class LandTeleportGUI {
public:
    LandTeleportGUI() = delete;

    LDAPI static void sendTo(Player& player); // sendTo -> impl

    LDAPI static void impl(Player& player, LandID id);
};


} // namespace land