#pragma once

#include "ll/api/mod/NativeMod.h"
#include "pland/hooks/EventListener.h"
#include "pland/infra/SafeTeleport.h"
#include "pland/land/LandScheduler.h"
#include <memory>

namespace mod {

class ModEntry {

public:
    static ModEntry& getInstance();

    ModEntry() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    /// @return True if the mod is unloaded successfully.
    bool unload();

    void onConfigReload();

    [[nodiscard]] land::SafeTeleport* getSafeTeleport() const { return mSafeTeleport.get(); }

private:
    ll::mod::NativeMod& mSelf;

    std::unique_ptr<land::EventListener> mEventListener;
    std::unique_ptr<land::LandScheduler> mLandScheduler;
    std::unique_ptr<land::SafeTeleport>  mSafeTeleport;

    friend class land::Require<land::LandScheduler>;
};

} // namespace mod
