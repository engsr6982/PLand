#include "About.h"

#include "core/WindowManager.h"

#include "imgui.h"
#include <utility>

namespace devtool::menus {

class AboutWindow final : public IWindow {
public:
    explicit AboutWindow(std::string title, DockSpec spec = {}) : IWindow(std::move(title), spec) {}

    void render() override {
        WindowScope scope(*this);
        if (!scope.isOpen()) {
            return;
        }

        ImGui::Text("PLand DevTools - 开发者工具");
        ImGui::Text("此工具用于快捷调试、查看、修改 PLand 的运行状态");
        ImGui::Text("\n\n");
        ImGui::Separator();
        ImGui::Text("Built with Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    }
};

About::About(WindowManager& wm) : IMenuElement("关于") {
    // 默认停靠: 主区右侧 25%
    auto& wnd = wm.create<AboutWindow>("关于", DockSpec{"", DockDirection::Right, 0.25F});
    this->setWindow(&wnd);
}

} // namespace devtool::menus
