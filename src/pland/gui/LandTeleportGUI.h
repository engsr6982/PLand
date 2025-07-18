#pragma once
#include "pland/Global.h"
#include "pland/land/Land.h"

class Player;

namespace land {


class LandTeleportGUI {
public:
    LandTeleportGUI() = delete;

    LDAPI static void sendTo(Player& player); // sendTo -> impl

    LDAPI static void impl(Player& player, SharedLand land);
};


} // namespace land