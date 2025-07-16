#pragma once

#include "ll/api/mod/NativeMod.h"
#include "pland/hooks/EventListener.h"
#include "pland/infra/SafeTeleport.h"
#include "pland/land/LandScheduler.h"
#include "pland/selector/SelectorManager.h"
#include <memory>

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

    LDNDAPI land::SafeTeleport* getSafeTeleport() const;
    LDNDAPI land::LandScheduler* getLandScheduler() const;
    LDNDAPI land::SelectorManager* getSelectorManager() const;

private:
    ll::mod::NativeMod& mSelf;

    std::unique_ptr<land::EventListener>   mEventListener;
    std::unique_ptr<land::LandScheduler>   mLandScheduler;
    std::unique_ptr<land::SafeTeleport>    mSafeTeleport;
    std::unique_ptr<land::SelectorManager> mSelectorManager;
};

} // namespace land
