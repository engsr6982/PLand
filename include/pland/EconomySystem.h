#pragma once
#include "Global.h"
#include "mc/world/actor/player/Player.h"
#include "utils/Utils.h"


namespace land {

class EconomySystem {
public:
    EconomySystem()                                = default;
    EconomySystem(const EconomySystem&)            = delete;
    EconomySystem& operator=(const EconomySystem&) = delete;
    EconomySystem(EconomySystem&&)                 = delete;
    EconomySystem& operator=(EconomySystem&&)      = delete;

    LDNDAPI static EconomySystem& getInstance();

    LDNDAPI long long get(Player& player);

    LDAPI bool set(Player& player, long long money);

    LDAPI bool add(Player& player, long long money);

    LDAPI bool reduce(Player& player, long long money);

    LDNDAPI string getSpendTip(Player& player, long long money);

    LDAPI void sendLackMoneyTip(Player& player, long long money); // 发送经济不足提示
};


} // namespace land