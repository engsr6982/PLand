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

        LDAPI static void recursionCalculationRefoundPrice(int& refundPrice, LandData_sptr const& ptr);
        LDAPI static void _deleteOrdinaryLandImpl(Player& player, LandData_sptr const& ptr); // 删除普通领地
        LDAPI static void _deleteSubLandImpl(Player& player, LandData_sptr const& ptr);      // 删除子领地
        LDAPI static void _deleteParentLandImpl(Player& player, LandData_sptr const& ptr);   // 删除父领地
        LDAPI static void _deleteMixLandImpl(Player& player, LandData_sptr const& ptr);      // 删除混合领地
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