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

    class ManageLandWithPlayer {
    public:
        LDAPI static void impl(Player& player, UUIDs const& targetPlayer);
    };


    // 辅助
    class IChoosePlayerFromDB {
    public:
        using ChoosePlayerCall = std::function<void(Player& self, UUIDs target)>;
        LDAPI static void impl(Player& player, ChoosePlayerCall callback);
    };
    class IChooseLand {
    public:
        LDAPI static void impl(Player& player, std::vector<LandData_sptr> const& lands);
    };
};


} // namespace land