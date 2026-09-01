#include "core/Window.h"
#include <utility>

namespace devtool {

IWindow::IWindow(std::string title, DockSpec spec) : title_(std::move(title)), dockSpec_(spec) {}

std::string const& IWindow::title() const { return title_; }

bool IWindow::visible() const { return visible_; }

void IWindow::setVisible(bool visible) { visible_ = visible; }

bool IWindow::enabled() const { return enabled_; }

void IWindow::setEnabled(bool enabled) { enabled_ = enabled; }

bool* IWindow::getVisibleFlag() { return &visible_; }

bool* IWindow::getEnabledFlag() { return &enabled_; }

DockSpec const& IWindow::dockSpec() const { return dockSpec_; }

ImGuiID IWindow::dockId() const { return lastDockId_; }

WindowScope::WindowScope(IWindow& window, ImGuiWindowFlags flags) {
    // ImGui 约定: *p_open == false(点击 X / 菜单取消勾选)后调用方不得再调用 Begin,
    // 否则窗口会重新显示。重新打开时 setVisible(true) 再 Begin, 自动恢复停靠。
    if (!window.visible()) {
        return;
    }
    if (!ImGui::Begin(window.title().data(), window.getVisibleFlag(), flags)) {
        ImGui::End();
        return;
    }
    begun_             = true;
    window.lastDockId_ = ImGui::GetWindowDockID();
}

WindowScope::~WindowScope() {
    if (begun_) {
        ImGui::End();
    }
}

} // namespace devtool
