#include "pland/economy/EconomySystem.h"
#include "pland/PLand.h"
#include "pland/land/Config.h"

#include <memory>
#include <stdexcept>

#include "econbridge/detail/LegacyMoneyEconomy.h"
#include "econbridge/detail/NullEconomy.h"
#include "econbridge/detail/ScoreboardEconomy.h"

#include <mc/world/actor/player/Player.h>

namespace land {


struct EconomySystem::Impl {
    std::shared_ptr<econbridge::IEconomy> mEconomyImpl;
    mutable std::mutex                    mInstanceMutex;

    std::shared_ptr<econbridge::IEconomy> create() const {
        auto& cfg = ConfigProvider::cfg.economy;
        if (!cfg.enabled) {
            PLand::getInstance().getSelf().getLogger().debug("using internals::EmptyEconomy");
            return std::make_shared<econbridge::detail::NullEconomy>();
        }

        switch (cfg.kit) {
        case EconomyKit::LegacyMoney: {
            PLand::getInstance().getSelf().getLogger().debug("using internals::LegacyMoneyEconomy");
            return std::make_shared<econbridge::detail::LegacyMoneyEconomy>();
        }
        case EconomyKit::ScoreBoard: {
            PLand::getInstance().getSelf().getLogger().debug("using internals::ScoreBoardEconomy");
            return std::make_shared<econbridge::detail::ScoreboardEconomy>(cfg.scoreboardName);
        }
        }

        throw std::runtime_error("Unknown econbridge kit, please check config!");
    }

    void initialize() {
        std::lock_guard lock(mInstanceMutex);
        mEconomyImpl = create();
    }

    void reload() {
        std::lock_guard lock(mInstanceMutex);
        mEconomyImpl.reset();
        mEconomyImpl = create();
    }
};

EconomySystem::EconomySystem() : impl(std::make_unique<Impl>()) {}

EconomySystem& EconomySystem::getInstance() {
    static EconomySystem instance;
    return instance;
}

void EconomySystem::initialize() { impl->initialize(); }
void EconomySystem::reload() { impl->reload(); }


std::shared_ptr<econbridge::IEconomy> EconomySystem::get() const { return impl->mEconomyImpl; }

bool EconomySystem::reduceChecked(mce::UUID const& uuid, int64_t amount) const {
    auto economy = get();
    if (economy->get(uuid) < amount) {
        return false;
    }
    return economy->reduce(uuid, amount);
}

std::string EconomySystem::getCostMessage(Player& player, llong amount) const {
    auto localeCode = player.getLocaleCode();

    auto& config = ConfigProvider::cfg.economy;
    if (!config.enabled) {
        return "\n[Tip] 经济系统未启用，本次操作不消耗 {}"_trl(localeCode, config.economyName);
    }

    llong currentMoney = get()->get(player.getUuid());
    bool  isEnough     = currentMoney >= amount;

    return "\n[Tip] 本次操作需要: {0} {1} | 当前余额: {2} | 剩余余额: {3} | {4}"_trl(
        localeCode,
        amount,
        config.economyName,
        currentMoney,
        currentMoney - amount,
        isEnough ? "余额充足"_trl(localeCode) : "余额不足"_trl(localeCode)
    );
}


} // namespace land
