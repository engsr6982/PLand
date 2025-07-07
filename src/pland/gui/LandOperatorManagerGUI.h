#pragma once
#include "pland/Global.h"
#include "pland/LandData.h"

class Player;

namespace land {


/**
 * @brief 领地操作员GUI
 */
class LandOperatorManagerGUI {
public:
    LDAPI static void sendMainMenu(Player& player);

    using ChoosePlayerCallback = std::function<void(Player& self, UUIDs const& target)>;
    LDAPI static void sendChoosePlayerFromDb(Player& player, ChoosePlayerCallback callback);

    LDAPI static void sendChooseLandGUI(Player& player, UUIDs const& targetPlayer);
    LDAPI static void sendChooseLandGUI(Player& player, std::vector<LandData_sptr> lands);
};


} // namespace land