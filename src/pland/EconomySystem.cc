#include "pland/EconomySystem.h"
#include "LLMoney.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/service/Bedrock.h"
#include "mc/world/level/Level.h"
#include "mc/world/scores/PlayerScoreSetFunction.h"
#include "mc/world/scores/ScoreInfo.h"
#include "pland/Config.h"
#include "pland/utils/McUtils.h"
#include "pland/utils/Utils.h"
#include <mc/world/actor/player/Player.h>
#include <mc/world/scores/Objective.h>
#include <mc/world/scores/Scoreboard.h>
#include <mc/world/scores/ScoreboardId.h>
#include <stdexcept>


namespace land {

int ScoreBoard_Get(Player& player, string const& scoreName) {
    Scoreboard& scoreboard = ll::service::getLevel()->getScoreboard();
    Objective*  obj        = scoreboard.getObjective(scoreName);
    if (!obj) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(
            player,
            "[Moneys] 插件错误: 找不到指定的计分板: {}"_trf(player, scoreName)
        );
        return 0;
    }
    ScoreboardId const& id = scoreboard.getScoreboardId(player);
    if (id.mRawID == ScoreboardId::INVALID().mRawID) {
        scoreboard.createScoreboardId(player);
    }
    return obj->getPlayerScore(id).mValue;
}

bool ScoreBoard_Set(Player& player, int score, string const& scoreName) {
    Scoreboard& scoreboard = ll::service::getLevel()->getScoreboard();
    Objective*  obj        = scoreboard.getObjective(scoreName);
    if (!obj) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(
            player,
            "[Moneys] 插件错误: 找不到指定的计分板: {}"_trf(player, scoreName)
        );
        return false;
    }
    const ScoreboardId& id = scoreboard.getScoreboardId(player);
    if (id.mRawID == ScoreboardId::INVALID().mRawID) {
        scoreboard.createScoreboardId(player);
    }
    bool isSuccess = false;
    scoreboard.modifyPlayerScore(isSuccess, id, *obj, score, PlayerScoreSetFunction::Set);
    return isSuccess;
}

bool ScoreBoard_Add(Player& player, int score, string const& scoreName) {
    Scoreboard& scoreboard = ll::service::getLevel()->getScoreboard();
    Objective*  obj        = scoreboard.getObjective(scoreName);
    if (!obj) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(
            player,
            "[Moneys] 插件错误: 找不到指定的计分板: {}"_trf(player, scoreName)
        );
        return false;
    }
    const ScoreboardId& id = scoreboard.getScoreboardId(player);
    if (id.mRawID == ScoreboardId::INVALID().mRawID) {
        scoreboard.createScoreboardId(player);
    }
    bool isSuccess = false;
    scoreboard.modifyPlayerScore(isSuccess, id, *obj, score, PlayerScoreSetFunction::Add);
    return isSuccess;
}

bool ScoreBoard_Reduce(Player& player, int score, string const& scoreName) {
    Scoreboard& scoreboard = ll::service::getLevel()->getScoreboard();
    Objective*  obj        = scoreboard.getObjective(scoreName);
    if (!obj) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(
            player,
            "[Moneys] 插件错误: 找不到指定的计分板: {}"_trf(player, scoreName)
        );
        return false;
    }
    const ScoreboardId& id = scoreboard.getScoreboardId(player);
    if (id.mRawID == ScoreboardId::INVALID().mRawID) {
        scoreboard.createScoreboardId(player);
    }
    bool isSuccess = false;
    scoreboard.modifyPlayerScore(isSuccess, id, *obj, score, PlayerScoreSetFunction::Subtract);
    return isSuccess;
}


// EconomySystem
EconomySystem& EconomySystem::getInstance() {
    static EconomySystem instance;
    return instance;
}


long long EconomySystem::get(Player& player) {
    switch (Config::cfg.economy.kit) {
    case EconomyKit::ScoreBoard:
        return ScoreBoard_Get(player, Config::cfg.economy.scoreboardObjName);
    case EconomyKit::LegacyMoney:
        return LLMoney_Get(player.getXuid());
    default:
        return 0;
    }
}


bool EconomySystem::set(Player& player, long long money) {
    switch (Config::cfg.economy.kit) {
    case EconomyKit::ScoreBoard:
        return ScoreBoard_Set(player, static_cast<int>(money), Config::cfg.economy.scoreboardObjName);
    case EconomyKit::LegacyMoney:
        return LLMoney_Set(player.getXuid(), money);
    default:
        return false;
    }
}


bool EconomySystem::add(Player& player, long long money) {
    if (!Config::cfg.economy.enabled) return true; // 未启用则不限制
    switch (Config::cfg.economy.kit) {
    case EconomyKit::ScoreBoard:
        return ScoreBoard_Add(player, static_cast<int>(money), Config::cfg.economy.scoreboardObjName);
    case EconomyKit::LegacyMoney:
        return LLMoney_Add(player.getXuid(), money);
    default:
        return false;
    }
}


bool EconomySystem::reduce(Player& player, long long money) {
    if (!Config::cfg.economy.enabled) return true; // 未启用则不限制
    if (get(player) >= money) {                    // 防止玩家余额不足
        switch (Config::cfg.economy.kit) {
        case EconomyKit::ScoreBoard:
            return ScoreBoard_Reduce(player, static_cast<int>(money), Config::cfg.economy.scoreboardObjName);
        case EconomyKit::LegacyMoney:
            return LLMoney_Reduce(player.getXuid(), money);
        default:
            return false;
        }
    }
    // 封装提示信息
    sendLackMoneyTip(player, money);
    return false;
}


string EconomySystem::getSpendTip(Player& player, long long money) {
    long long currentMoney = Config::cfg.economy.enabled ? get(player) : 0;
    string    prefix       = "\n[§uTip§r]§r ";

    auto& name = Config::cfg.economy.economyName;

    if (Config::cfg.economy.enabled)
        return prefix
             + "此操作消耗§6{0}§r:§e{1}§r | 当前§6{2}§r:§d{3}§r | 剩余§6{4}§r:§s{5}§r | {6}"_trf(
                   player,
                   name,
                   money,
                   name,
                   currentMoney,
                   name,
                   currentMoney - money,
                   currentMoney >= money ? "§6{}§r§a充足§r"_trf(player, name) : "§6{}§r§c不足§r"_trf(player, name)
             );
    else return prefix + "经济系统未启用，此操作不消耗§6{0}§r"_trf(player, name);
}

void EconomySystem::sendLackMoneyTip(Player& player, long long money) {
    mc_utils::sendText<mc_utils::LogLevel::Error>(
        player,
        "[Moneys] 操作失败，此操作需要{0}:{1}，当前{2}:{3}"_trf(
            player,
            Config::cfg.economy.economyName,
            money,
            Config::cfg.economy.economyName,
            get(player)
        )
    );
}

} // namespace land