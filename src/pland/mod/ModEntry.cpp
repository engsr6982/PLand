#include "pland/mod/ModEntry.h"

#include <memory>

#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/utils/SystemUtils.h"

#include "ll/api/io/LogLevel.h"
#include "pland/Command.h"
#include "pland/Config.h"
#include "pland/EventListener.h"
#include "pland/Global.h"
#include "pland/LandScheduler.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/Version.h"

#ifdef LD_TEST
#include "LandEventTest.h"
#endif

#ifdef LD_DEVTOOL
#include "DevToolAppManager.h"
#endif

namespace mod {

ModEntry& ModEntry::getInstance() {
    static ModEntry instance;
    return instance;
}

bool ModEntry::load() {
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

    land::PLand::getInstance().init();

#ifdef DEBUG
    logger.warn("Debug Mode");
    logger.setLevel(ll::io::LogLevel::Trace);
#endif

    return true;
}

bool ModEntry::enable() {
    land::LandCommand::setup();

    this->mLandScheduler = std::make_unique<land::LandScheduler>();
    this->mEventListener = std::make_unique<land::EventListener>();


#ifdef LD_TEST
    test::SetupEventListener();
#endif

#ifdef LD_DEVTOOL
    if (land::Config::cfg.internal.devTools) devtool::DevToolAppManager::getInstance().initApp();
#endif

    return true;
}

bool ModEntry::disable() {
#ifdef LD_DEVTOOL
    if (land::Config::cfg.internal.devTools) devtool::DevToolAppManager::getInstance().destroyApp();
#endif

    auto& logger = getSelf().getLogger();
    logger.info("Stopping thread and saving data...");
    land::PLand::getInstance().stopThread(); // 请求关闭线程
    logger.debug("[Main] Saving land data...");
    land::PLand::getInstance().save();
    logger.debug("[Main] Land data saved.");

    logger.debug("Stopping coroutine...");
    land::GlobalRepeatCoroTaskRunning.store(false);

    logger.debug("cleaning up...");
    land::SelectorManager::getInstance().cleanup();

    this->mLandScheduler.reset();
    mEventListener.reset();

    return true;
}

bool ModEntry::unload() { return true; }

void ModEntry::onConfigReload() {
    auto& logger = getSelf().getLogger();
    logger.trace("Reloading event listener...");
    try {
        mEventListener.reset();
        logger.trace("Event listener reset, creating new instance...");
        mEventListener = std::make_unique<land::EventListener>();
        logger.trace("Event listener reloaded successfully.");
    } catch (std::exception const& e) {
        getSelf().getLogger().error("Failed to reload event listener: {}", e.what());
    } catch (...) {
        getSelf().getLogger().error("Failed to reload event listener: unknown error");
    }
}


} // namespace mod

LL_REGISTER_MOD(mod::ModEntry, mod::ModEntry::getInstance());
