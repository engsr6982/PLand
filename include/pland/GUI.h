#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandSelector.h"
#include "pland/wrapper/FormEx.h"
#include <functional>
#include <vector>

namespace land {
using namespace wrapper;

// 独立GUI
class ChooseLandDimAndNewLand {
public:
    LDAPI static void impl(Player& player);
};
class SelectorChangeYGui {
public:
    LDAPI static void impl(Player& player, std::string const& exception = "");
};

// 主界面
class LandMainGui {
public:
    LDAPI static void impl(Player& player);
};


class EditPlayerSettingGui {
public:
    LDAPI static void impl(Player& player);
};


// 领地管理GUI


// 领地成员管理GUI
class EditLandMemberGui {
public:
    LDAPI static void impl(Player& player, LandData_sptr ptr);

    class AddMemberGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr ptr);
    };

    class RemoveMemberGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr ptr, UUIDs members);
    };
};


// 领地传送GUI
class LandTeleportGui {
public:
    LDAPI static void impl(Player& player); // ChooseLandGui -> TeleportGui::run

    LDAPI static void run(Player& player, LandID id);
};


// 管理员GUI
class LandOPManagerGui {
public:
    LDAPI static void impl(Player& player);

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