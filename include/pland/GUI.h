#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/wrapper/FormEx.h"
#include <functional>
#include <vector>

namespace land {
using namespace wrapper;

// 独立GUI
class LandBuyGui {
public:
    LDAPI static void impl(Player& player);

    class LandBuyWithReSelectGui {
    public:
        LDAPI static void impl(Player& player);
    };
};
class ChooseLandDimAndNewLand {
public:
    LDAPI static void impl(Player& player);
};
class SelectorChangeYGui {
public:
    LDAPI static void impl(Player& player, std::string const& exception = "");
};


// 通用组件Gui
class ChooseLandUtilGui {
public:
    using ChooseCallback = std::function<void(Player&, LandID id)>;
    template <typename ParentForm = void>
    LDAPI static void impl(Player& player, ChooseCallback const& callback, bool showShredLand = false);
};
class ChoosePlayerUtilGui {
public:
    using ChoosePlayerCall = std::function<void(Player&, Player& choosedPlayer)>;
    template <typename ParentForm = void>
    LDAPI static void impl(Player& player, ChoosePlayerCall const& callback);
};
class EditStringUtilGui {
public:
    using EditStringResult = std::function<void(Player& self, string result)>;
    LDAPI static void impl(
        Player&                 player,
        string const&           title,       // 标题
        string const&           text,        // 提示
        string const&           defaultValu, // 默认值
        EditStringResult const& callback     // 回调
    );
};
class FuzzySerarchUtilGui {
public:
    using CallBack = std::function<void(Player& slef, std::vector<LandData_sptr> const& result)>;
    LDAPI static void impl(Player& player, std::vector<LandData_sptr> const& list, CallBack const& callback);
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
class LandManagerGui {
public:
    LDAPI static void impl(Player& player, LandID id);

    class EditLandPermGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr const& ptr);
    };

    class DeleteLandGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr const& ptr);
    };

    class EditLandNameGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr const& ptr);
    };
    class EditLandDescGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr const& ptr);
    };
    class EditLandOwnerGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr const& ptr);
    };
    class ReSelectLandGui {
    public:
        LDAPI static void impl(Player& player, LandData_sptr const& ptr);
    };
};


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