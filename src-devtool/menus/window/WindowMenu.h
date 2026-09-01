#pragma once
#include "core/Menu.h"

namespace devtool {

class WindowManager;

class WindowMenu final : public IMenu {
public:
    WindowMenu();

    void render() override;

    void onAttached(WindowManager& wm) override;
};

} // namespace devtool
