#pragma once
#include <imgui.h>
#include <string>
#include <string_view>

namespace devtool {

enum class DockDirection { None, Left, Right, Up, Down, Tab };

// 初始停靠描述; 仅首次启动(无有效 imgui.ini)时用于构建默认布局
struct DockSpec {
    std::string_view parent;                        // 相对停靠的宿主窗口标题; "" = 主停靠区中央
    DockDirection    direction{DockDirection::Tab}; // 停靠方向
    float            sizeRatio{0.5f};               // 仅 Left/Right/Up/Down 生效
};

class WindowScope;


class IWindow {
protected:
    std::string title_; // 稳定的窗口标题(生命周期内不变, 作为 ini/停靠的键)
    bool        visible_{false};
    bool        enabled_{true};
    DockSpec    dockSpec_;
    ImGuiID     lastDockId_{0}; // 由 WindowScope 在 Begin 后写入

public:
    explicit IWindow(std::string title, DockSpec spec = {});
    virtual ~IWindow() = default;

    std::string const& title() const;
    bool               visible() const;
    void               setVisible(bool visible);
    bool               enabled() const;
    void               setEnabled(bool enabled);
    bool*              getVisibleFlag();
    bool*              getEnabledFlag();
    DockSpec const&    dockSpec() const;
    ImGuiID            dockId() const; // 0 = 未停靠(悬浮)

    virtual void render() = 0;

    friend class WindowScope;
};

class WindowScope {
    bool begun_{false};

public:
    WindowScope(IWindow& window, ImGuiWindowFlags flags = 0);
    ~WindowScope();
    WindowScope(WindowScope const&)            = delete;
    WindowScope& operator=(WindowScope const&) = delete;

    bool isOpen() const { return begun_; }
};

} // namespace devtool
