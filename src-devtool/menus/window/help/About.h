#pragma once
#include "core/Menu.h"

namespace devtool {
class WindowManager;
}

namespace devtool::menus {

class About final : public IMenuElement {
public:
    explicit About(WindowManager& wm);
};

} // namespace devtool::menus
