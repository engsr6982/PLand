#pragma once
#include "core/Window.h"
#include <concepts>
#include <imgui.h>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace devtool {


class WindowManager {
    std::vector<std::unique_ptr<IWindow>>     windows_; // 所有权(注册顺序即渲染顺序)
    std::unordered_map<std::string, IWindow*> byTitle_; // 标题索引(ImGui 要求标题唯一)

    ImGuiID dockspaceId_{0};
    bool    layoutInitialized_{false};

    // 晚注册窗口(如 LandEditor)的定向停靠请求, 帧末处理
    struct PendingDock {
        IWindow* host;
        IWindow* target;
        int      attempts{0};
    };
    std::vector<PendingDock>              pendingDocks_;
    std::vector<std::unique_ptr<IWindow>> pendingDestroy_; // 帧末延迟销毁

    IWindow& registerOwnedImpl(std::unique_ptr<IWindow> window);

public:
    WindowManager()                                = default;
    ~WindowManager()                               = default;
    WindowManager(WindowManager const&)            = delete;
    WindowManager& operator=(WindowManager const&) = delete;

    // 注册窗口(接管所有权)并返回引用; 仅允许在 render 线程启动前调用
    template <typename T, typename... Args>
        requires std::derived_from<T, IWindow>
    T& create(std::string title, Args&&... args) {
        return static_cast<T&>(registerOwnedImpl(std::make_unique<T>(std::move(title), std::forward<Args>(args)...)));
    }
    // 注册已构造的窗口(用于 LandEditor/LandTreeViewer 等动态窗口, 标题由构造内部确定); 返回具体类型引用
    template <typename T>
        requires std::derived_from<T, IWindow>
    T& registerOwned(std::unique_ptr<T> window) {
        return static_cast<T&>(registerOwnedImpl(std::move(window)));
    }
    // 注销窗口, 帧末销毁
    void     unregister(IWindow* window);
    IWindow* find(std::string_view title) const;

    // 主停靠区 ID
    [[nodiscard]] ImGuiID dockspaceId() const { return dockspaceId_; }

    // —— render 线程管线 ——
    void frameBegin(ImGuiID dockspaceId);                 // DockSpaceOverViewport + 首帧默认布局
    void renderAll();                                     // 平铺渲染全部窗口
    void processDockRequests();                           // 处理定向停靠请求(帧内全部 Begin/End 结束后)
    void postFrame();                                     // 延迟销毁
    void requestDockInto(IWindow* host, IWindow* target); // 目标停靠进宿主当前节点(窗口内可调用, 仅记录意图)

    void buildLayoutFromSpecs(); // DockBuilder 重建默认布局(幂等)
    void resetLayout();          // 删 ini + 重建默认布局 + 立即落盘
};

} // namespace devtool
