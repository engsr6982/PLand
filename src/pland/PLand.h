#pragma once
#include <memory>

#include "ll/api/mod/NativeMod.h"

#include "pland/hooks/EventListener.h"
#include "pland/infra/SafeTeleport.h"
#include "pland/land/LandRegistry.h"
#include "pland/land/LandScheduler.h"
#include "pland/selector/SelectorManager.h"


namespace land {

class PLand {
    PLand();

public: /* private */
    [[nodiscard]] ll::mod::NativeMod& getSelf() const;

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    /// @return True if the mod is unloaded successfully.
    bool unload();

public: /* public */
    LDAPI static PLand& getInstance();

    LDAPI void onConfigReload();

    LDNDAPI SafeTeleport*    getSafeTeleport() const;
    LDNDAPI LandScheduler*   getLandScheduler() const;
    LDNDAPI SelectorManager* getSelectorManager() const;
    LDNDAPI LandRegistry*    getLandRegistry() const;

private:
    ll::mod::NativeMod& mSelf;

    std::unique_ptr<LandRegistry>    mLandRegistry;
    std::unique_ptr<EventListener>   mEventListener;
    std::unique_ptr<LandScheduler>   mLandScheduler;
    std::unique_ptr<SafeTeleport>    mSafeTeleport;
    std::unique_ptr<SelectorManager> mSelectorManager;
};

} // namespace land
