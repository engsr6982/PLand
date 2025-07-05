#pragma once
#include "pland/Global.h"
#include "pland/LandSelector.h"


namespace land {

class BuyLandGui {
public:
    /**
     * @brief 购买领地(调用对应的impl函数)
     */
    LDAPI static void impl(Player& player);

    /**
     * @brief 购买普通领地
     */
    LDAPI static void impl(Player& player, Selector* selector);

    /**
     * @brief 购买新的领地范围
     */
    LDAPI static void impl(Player& player, LandReSelector* reSelector);

    /**
     * @brief 购买子领地
     */
    LDAPI static void impl(Player& player, SubLandSelector* subSelector);
};

} // namespace land
