#include "pland/PLand.h"

#include <memory>

#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/utils/SystemUtils.h"

#include "ll/api/io/LogLevel.h"
#include "pland/Global.h"
#include "pland/Version.h"
#include "pland/command/Command.h"
#include "pland/economy/EconomySystem.h"
#include "pland/hooks/EventListener.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"
#include "pland/land/LandScheduler.h"
#include "pland/selector/SelectorManager.h"


#ifdef LD_TEST
#include "TestMain.h"
#endif

#ifdef LD_DEVTOOL
#include "DevToolAppManager.h"
#endif

namespace mod {

PLand& PLand::getInstance() {
    static PLand instance;
    return instance;
}

bool PLand::load() {
    auto& logger = getSelf().getLogger();
    logger.info(R"(  _____   _                        _ )");
    logger.info(R"( |  __ \ | |                      | |)");
    logger.info(R"( | |__) || |      __ _  _ __    __| |)");
    logger.info(R"( |  ___/ | |     / _` || '_ \  / _` |)");
    logger.info(R"( | |     | |____| (_| || | | || (_| |)");
    logger.info(R"( |_|     |______|\__,_||_| |_| \__,_|)");
    logger.info(R"(                                     )");
    logger.info("Loading...");

    if (PLAND_VERSION_SNAPSHOT) {
        logger.warn("Version: {}", PLAND_VERSION_STRING);
        logger.warn("您当前正在使用开发快照版本，此版本可能某些功能异常、损坏、甚至导致崩溃，请勿在生产环境中使用。");
        logger.warn("You are using a development snapshot version, this version may have some abnormal, broken or even "
                    "crash functions, please do not use it in production environment.");
    } else {
        logger.info("Version: {}", PLAND_VERSION_STRING);
    }

    if (auto res = ll::i18n::getInstance().load(getSelf().getLangDir()); !res) {
        logger.error("Load language file failed, plugin will use default language.");
        res.error().log(logger);
    }

    land::Config::tryLoad();
    logger.setLevel(land::Config::cfg.logLevel);

    land::LandRegistry::getInstance().init();
    land::EconomySystem::getInstance().initEconomySystem();

#ifdef DEBUG
    logger.warn("Debug Mode");
    logger.setLevel(ll::io::LogLevel::Trace);
#endif

    return true;
}

bool PLand::enable() {
    land::LandCommand::setup();

    this->mLandScheduler   = std::make_unique<land::LandScheduler>();
    this->mEventListener   = std::make_unique<land::EventListener>();
    this->mSafeTeleport    = std::make_unique<land::SafeTeleport>();
    this->mSelectorManager = std::make_unique<land::SelectorManager>();


#ifdef LD_TEST
    test::TestMain::setup();
#endif

#ifdef LD_DEVTOOL
    if (land::Config::cfg.internal.devTools) devtool::DevToolAppManager::getInstance().initApp();
#endif

    return true;
}

bool PLand::disable() {
#ifdef LD_DEVTOOL
    if (land::Config::cfg.internal.devTools) devtool::DevToolAppManager::getInstance().destroyApp();
#endif

    auto& logger = getSelf().getLogger();
    logger.info("Stopping thread and saving data...");
    land::LandRegistry::getInstance().stopThread(); // 请求关闭线程
    logger.debug("[Main] Saving land data...");
    land::LandRegistry::getInstance().save();
    logger.debug("[Main] Land data saved.");

    logger.debug("Stopping coroutine...");
    land::GlobalRepeatCoroTaskRunning.store(false);

    mLandScheduler.reset();
    mEventListener.reset();
    mSafeTeleport.reset();
    mSelectorManager.reset();

    return true;
}

bool PLand::unload() { return true; }

void PLand::onConfigReload() {
    auto& logger = getSelf().getLogger();
    logger.trace("Reloading event listener...");
    try {
        mEventListener.reset();
        logger.trace("Event listener reset, creating new instance...");
        mEventListener = std::make_unique<land::EventListener>();
        logger.trace("Event listener reloaded successfully.");

        logger.trace("Reloading economy system...");
        land::EconomySystem::getInstance().reloadEconomySystem();
        logger.trace("Economy system reloaded successfully.");
    } catch (std::exception const& e) {
        getSelf().getLogger().error("Failed to reload event listener: {}", e.what());
    } catch (...) {
        getSelf().getLogger().error("Failed to reload event listener: unknown error");
    }
}

PLand::PLand() : mSelf(*ll::mod::NativeMod::current()) {}
ll::mod::NativeMod&    PLand::getSelf() const { return mSelf; }
land::SafeTeleport*    PLand::getSafeTeleport() const { return mSafeTeleport.get(); }
land::LandScheduler*   PLand::getLandScheduler() const { return mLandScheduler.get(); }
land::SelectorManager* PLand::getSelectorManager() const { return mSelectorManager.get(); }


} // namespace mod

LL_REGISTER_MOD(mod::PLand, mod::PLand::getInstance());
