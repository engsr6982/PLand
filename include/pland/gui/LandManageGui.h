#pragma once
#include "pland/Global.h"
#include "pland/LandData.h"


namespace land {


class LandManageGui {
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


} // namespace land