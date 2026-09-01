#pragma once
#include <concepts>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace devtool {

class IWindow;
class WindowManager;

// 菜单栏中的单个条目; 可选绑定一个窗口(非拥有), 勾选状态即窗口可见性
class IMenuElement {
protected:
    std::string label_;
    std::string shortCut_;
    bool        enable_{true};
    bool        selected_{false};
    IWindow*    window_{nullptr};

public:
    explicit IMenuElement(std::string label);
    IMenuElement(std::string label, std::string shortCut);
    virtual ~IMenuElement() = default;

    std::string const& getLabel() const;
    std::string const& getShortCut() const;
    bool               isEnable() const;
    bool               isSelected() const;
    bool*              getEnableFlag();
    bool*              getSelectFlag(); // 绑定窗口时返回窗口可见性标志, 否则返回自身 selected_

    void setWindow(IWindow* window);

    virtual void render();
};


class IMenu {
protected:
    std::unordered_map<std::string, std::unique_ptr<IMenuElement>> elements_;
    std::string                                                    label_;
    bool                                                           enable_{true};

    void registerElementImpl(std::unique_ptr<IMenuElement> element);

public:
    explicit IMenu(std::string label);
    virtual ~IMenu() = default;

    std::string const& getLabel() const;
    bool               isEnable() const;
    bool*              getEnableFlag();

    virtual void render();

    // 初始化钩子: DevToolApp 注册菜单时注入 WindowManager 并调用(仅 render 线程启动前一次);
    // 子类在此把 wm 传给 registerElement, 由各元素自注册自己的窗口; 默认无操作
    virtual void onAttached(WindowManager&) {}

    template <typename T, typename... Args>
        requires std::derived_from<T, IMenuElement> && std::is_final_v<T>
    void registerElement(Args&&... args) {
        registerElementImpl(std::make_unique<T>(std::forward<Args>(args)...));
    }
};

} // namespace devtool
