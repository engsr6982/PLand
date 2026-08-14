#pragma once
#include "components/CodeEditor.h"
#include <memory>

namespace land {
class Land;
}
namespace devtool::menus {
class LandCacheViewerWindow;
}
namespace devtool::viewer {

class LandEditor : public CodeEditor {
    std::weak_ptr<land::Land> land_;

    friend class devtool::menus::LandCacheViewerWindow;

public:
    explicit LandEditor(std::shared_ptr<land::Land> land);

    void renderMenuElement() override;
};

} // namespace devtool::viewer