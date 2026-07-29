#include "pland/PLand.h"
#include "BuildInfo.h"

#include <memory>

#include "ll/api/Versions.h"
#include "ll/api/data/Version.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/io/LogLevel.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/thread/ThreadPoolExecutor.h"
#include "ll/api/utils/SystemUtils.h"

#include "drawer/DrawHandleManager.h"
#include "events/domain/ConfigReloadEvent.h"
#include "internal/interceptor/InterceptorConfig.h"
#include "land/internal/LandScheduler.h"
#include "land/internal/SafeTeleport.h"
#include "pland/economy/EconomySystem.h"
#include "pland/internal/adapter/telemetry/Telemetry.h"
#include "pland/internal/command/Command.h"
#include "pland/internal/interceptor/EventInterceptor.h"
#include "pland/land/Config.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/selector/SelectorManager.h"
#include "pland/service/ServiceLocator.h"


#ifdef LD_DEVTOOL
#include "DevToolApp.h"
#endif

namespace land {


struct PLand::Impl {
    ll::mod::NativeMod&                                      mSelf;
    std::unique_ptr<ll::thread::ThreadPoolExecutor>          mThreadPoolExecutor{nullptr};
    std::unique_ptr<LandRegistry>                            mLandRegistry{nullptr};
    std::unique_ptr<internal::LandScheduler>                 mLandScheduler{nullptr};
    std::unique_ptr<internal::interceptor::EventInterceptor> mEventListener{nullptr};
    std::unique_ptr<internal::SafeTeleport>                  mSafeTeleport{nullptr};
    std::unique_ptr<SelectorManager>                         mSelectorManager{nullptr};
    std::unique_ptr<DrawHandleManager>                       mDrawHandleManager{nullptr};
    std::unique_ptr<internal::adapter::Telemetry>            mTelemetry{nullptr};

    ll::event::ListenerPtr mConfigReloadListener{nullptr};

    std::unique_ptr<service::ServiceLocator> mServiceLocator{nullptr};

#ifdef LD_DEVTOOL
    std::unique_ptr<devtool::DevToolApp> mDevToolApp{nullptr};
#endif

    explicit Impl() : mSelf(*ll::mod::NativeMod::current()) {}
};

bool ensureStableVersion() {
    auto tag = land::BuildInfo::Tag;
    if (tag.find("-g") != std::string_view::npos) {
        return false;
    }
    if (!tag.empty() && tag.front() == 'v') {
        tag.remove_prefix(1);
    }
    return ll::data::Version::valid(tag);
}

bool PLand::load() {
    auto& logger = getSelf().getLogger();
    logger.info("{}-{}-{}", BuildInfo::Tag, BuildInfo::Branch, BuildInfo::Commit);
    if (!ensureStableVersion()) {
        logger.warn("This is a development build ({}). It may not be stable.", BuildInfo::Tag);
    }

    if (auto res = ll::i18n::getInstance().load(getSelf().getLangDir()); !res) {
        logger.error("Load language file failed, plugin will use default language.");
        res.error().log(logger);
    }

    internal::interceptor::InterceptorConfig::tryMigrateLegacyConfig(getSelf().getConfigDir());

    loadConfig();
    internal::interceptor::InterceptorConfig::load(getSelf().getConfigDir());

    mImpl->mThreadPoolExecutor = std::make_unique<ll::thread::ThreadPoolExecutor>("PLand-ThreadPool", 2);

    try {
        mImpl->mLandRegistry = std::make_unique<land::LandRegistry>(*this);

        EconomySystem::getInstance().initialize();
    } catch (std::exception const& exception) {
        logger.error(exception.what());
        mImpl->mThreadPoolExecutor->destroy(); // fix deadlock
        return false;
    }

#ifdef DEBUG
    logger.warn("Debug Mode");
    logger.setLevel(ll::io::LogLevel::Trace);
#endif

    return true;
}

bool PLand::enable() {
    internal::LandCommand::setup();
    mImpl->mLandScheduler     = std::make_unique<internal::LandScheduler>();
    mImpl->mEventListener     = std::make_unique<internal::interceptor::EventInterceptor>();
    mImpl->mSafeTeleport      = std::make_unique<internal::SafeTeleport>();
    mImpl->mSelectorManager   = std::make_unique<SelectorManager>();
    mImpl->mDrawHandleManager = std::make_unique<DrawHandleManager>();
    mImpl->mTelemetry         = std::make_unique<internal::adapter::Telemetry>();
    if (ConfigProvider::isTelemetryEnabled()) {
        mImpl->mTelemetry->launch(getThreadPool());
    }

    mImpl->mServiceLocator = std::make_unique<service::ServiceLocator>(*this);

    mImpl->mConfigReloadListener = ll::event::EventBus::getInstance().emplaceListener<events::ConfigReloadEvent>(
        [this](events::ConfigReloadEvent& ev [[maybe_unused]]) {
            internal::interceptor::InterceptorConfig::load(getSelf().getConfigDir());
            mImpl->mEventListener.reset();
            mImpl->mEventListener = std::make_unique<internal::interceptor::EventInterceptor>();

            EconomySystem::getInstance().reload();

            if (ConfigProvider::isTelemetryEnabled()) {
                mImpl->mTelemetry->launch(getThreadPool());
            } else {
                mImpl->mTelemetry->shutdown();
            }

            mImpl->mDrawHandleManager.reset();
            mImpl->mDrawHandleManager = std::make_unique<DrawHandleManager>();
        }
    );

#ifdef LD_DEVTOOL
    if (ConfigProvider::isDevToolsEnabled()) {
        mImpl->mDevToolApp = devtool::DevToolApp::make();
    }
#endif

    return true;
}

bool PLand::disable() {
#ifdef LD_DEVTOOL
    if (ConfigProvider::isDevToolsEnabled()) {
        mImpl->mDevToolApp.reset();
    }
#endif
    ll::event::EventBus::getInstance().removeListener(mImpl->mConfigReloadListener);

    auto& logger = mImpl->mSelf.getLogger();
    mImpl->mTelemetry.reset();

    mImpl->mServiceLocator.reset();

    logger.debug("Saving land registry...");
    mImpl->mLandRegistry->save();

    logger.debug("Destroying resources...");
    mImpl->mLandScheduler.reset();
    mImpl->mEventListener.reset();
    mImpl->mSafeTeleport.reset();
    mImpl->mSelectorManager.reset();
    mImpl->mDrawHandleManager.reset();
    mImpl->mLandRegistry.reset();

    logger.debug("Destroying thread pool...");
    mImpl->mThreadPoolExecutor->destroy();
    mImpl->mThreadPoolExecutor.reset();
    return true;
}

bool PLand::unload() { return true; }

PLand& PLand::getInstance() {
    static PLand instance;
    return instance;
}

PLand::PLand() : mImpl(std::make_unique<Impl>()) {}

ll::mod::NativeMod&     PLand::getSelf() const { return mImpl->mSelf; }
SelectorManager*        PLand::getSelectorManager() const { return mImpl->mSelectorManager.get(); }
LandRegistry&           PLand::getLandRegistry() const { return *mImpl->mLandRegistry; }
DrawHandleManager*      PLand::getDrawHandleManager() const { return mImpl->mDrawHandleManager.get(); }
internal::SafeTeleport& PLand::getSafeTeleport() const { return *mImpl->mSafeTeleport; }
bool                    PLand::loadConfig() {
    if (auto expected = ConfigProvider::load(getSelf().getConfigDir())) {
        return true;
    } else {
        auto& logger = mImpl->mSelf.getLogger();
        expected.error().log(logger);
        return false;
    }
}
bool PLand::saveConfig() {
    if (auto expected = ConfigProvider::save(getSelf().getConfigDir())) {
        return true;
    } else {
        auto& logger = mImpl->mSelf.getLogger();
        expected.error().log(logger);
        return false;
    }
}

ll::thread::ThreadPoolExecutor& PLand::getThreadPool() const { return *mImpl->mThreadPoolExecutor; }
service::ServiceLocator&        PLand::getServiceLocator() const { return *mImpl->mServiceLocator; }

#ifdef LD_DEVTOOL
void PLand::setDevToolVisible(bool visible) {
    if (ConfigProvider::isDevToolsEnabled()) {
        if (visible) {
            mImpl->mDevToolApp->show();
        } else {
            mImpl->mDevToolApp->hide();
        }
    }
}
#endif

} // namespace land

LL_REGISTER_MOD(land::PLand, land::PLand::getInstance());
