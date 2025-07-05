#pragma once

#include "ll/api/mod/NativeMod.h"
#include "pland/EventListener.h"
#include "pland/LandScheduler.h"
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

private:
    ll::mod::NativeMod& mSelf;

    std::unique_ptr<land::EventListener> mEventListener;
    std::unique_ptr<land::LandScheduler> mLandScheduler;

    friend class land::Require<land::LandScheduler>;
};

} // namespace mod
