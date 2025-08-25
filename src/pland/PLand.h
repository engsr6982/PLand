#pragma once
#include <memory>

#include "Global.h"
#include "ll/api/mod/NativeMod.h"


namespace land {

class LandRegistry;
class EventListener;
class LandScheduler;
class SafeTeleport;
class SelectorManager;
class DrawHandleManager;

class PLand {
    PLand();

public: /* private */
    [[nodiscard]] ll::mod::NativeMod& getSelf() const;

    bool load();
    bool enable();
    bool disable();
    bool unload();

public: /* public */
    LDAPI static PLand& getInstance();

    LDAPI void onConfigReload();

    LDNDAPI SafeTeleport*      getSafeTeleport() const;
    LDNDAPI LandScheduler*     getLandScheduler() const;
    LDNDAPI SelectorManager*   getSelectorManager() const;
    LDNDAPI LandRegistry*      getLandRegistry() const;
    LDNDAPI DrawHandleManager* getDrawHandleManager() const;

private:
    ll::mod::NativeMod& mSelf;

    std::unique_ptr<LandRegistry>      mLandRegistry;
    std::unique_ptr<EventListener>     mEventListener;
    std::unique_ptr<LandScheduler>     mLandScheduler;
    std::unique_ptr<SafeTeleport>      mSafeTeleport;
    std::unique_ptr<SelectorManager>   mSelectorManager;
    std::unique_ptr<DrawHandleManager> mDrawHandleManager;
};

} // namespace land
