#pragma once
#include <string>

namespace land {


enum class EconomyKit { LegacyMoney, ScoreBoard };

struct EconomyConfig {
    bool        enabled        = false;
    EconomyKit  kit            = EconomyKit::LegacyMoney;
    std::string scoreboardName = "Scoreboard";
    std::string economyName    = "Coin";
};


} // namespace land