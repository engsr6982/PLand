#include "WindowMenu.h"

#include "menus/window/help/About.h"
#include "menus/window/land/LandCacheViewer.h"

#include <imgui.h>
#include <ranges>

namespace devtool {

WindowMenu::WindowMenu() : IMenu("窗口") {}

void WindowMenu::onAttached(WindowManager& wm) {
    registerElement<menus::LandCacheViewer>(wm);
    registerElement<menus::About>(wm);
}

void WindowMenu::render() {
    if (ImGui::BeginMenu(label_.data(), getEnableFlag())) {
        bool first = true;
        for (auto const& element : elements_ | std::views::values) {
            if (!first) {
                ImGui::Separator();
            }
            first = false;
            element->render();
        }
        ImGui::EndMenu();
    }
}

} // namespace devtool
