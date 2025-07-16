#include "pland/economy/EconomySystem.h"
#include "pland/PLand.h"
#include "pland/economy/impl/EmtpyInterface.h"
#include "pland/economy/impl/LegacyMoneyInterface.h"
#include "pland/economy/impl/ScoreBoardInterface.h"
#include "pland/infra/Config.h"
#include <memory>
#include <stdexcept>



namespace land {


std::shared_ptr<internals::IEconomyInterface> EconomySystem::createEconomySystem() const {
    auto& cfg = getConfig();
    if (!cfg.enabled) {
        mod::PLand::getInstance().getSelf().getLogger().debug("using internals::EmtpyInterface");
        return std::make_shared<internals::EmtpyInterface>();
    }

    switch (cfg.kit) {
    case land::EconomyKit::LegacyMoney: {
        mod::PLand::getInstance().getSelf().getLogger().debug("using internals::LegacyMoneyInterface");
        return std::make_shared<internals::LegacyMoneyInterface>();
    }
    case land::EconomyKit::ScoreBoard: {
        mod::PLand::getInstance().getSelf().getLogger().debug("using internals::ScoreBoardInterface");
        return std::make_shared<internals::ScoreBoardInterface>();
    }
    }

    throw std::runtime_error("Unknown economy kit, please check config!");
}

EconomySystem& EconomySystem::getInstance() {
    static EconomySystem instance;
    return instance;
}

std::shared_ptr<internals::IEconomyInterface> EconomySystem::getEconomyInterface() const {
    std::lock_guard<std::mutex> lock(mInstanceMutex);
    if (!mEconomySystem) {
        throw std::runtime_error("internals::IEconomyInterface not initialized.");
    }
    return mEconomySystem;
}

EconomyConfig& EconomySystem::getConfig() const { return Config::cfg.economy; }

void EconomySystem::initEconomySystem() {
    std::lock_guard<std::mutex> lock(mInstanceMutex);
    if (!mEconomySystem) {
        mEconomySystem = createEconomySystem();
        return;
    }
}
void EconomySystem::reloadEconomySystem() {
    std::lock_guard<std::mutex> lock(mInstanceMutex);
    mEconomySystem = createEconomySystem();
}


EconomySystem::EconomySystem() = default;

std::shared_ptr<internals::IEconomyInterface> EconomySystem::operator->() const { return mEconomySystem; }


} // namespace land
