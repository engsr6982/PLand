#pragma once
#include "core/Menu.h"

namespace devtool {
class WindowManager;
}

namespace devtool::menus {

class LandCacheViewer final : public IMenuElement {
public:
    explicit LandCacheViewer(WindowManager& wm);
};

} // namespace devtool::menus
