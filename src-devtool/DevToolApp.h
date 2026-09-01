#pragma once
#include "core/Menu.h"
#include <concepts>
#include <memory>
#include <string>

namespace devtool {

class DevToolApp final {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    explicit DevToolApp();
    ~DevToolApp();

    DevToolApp(DevToolApp&)                  = delete;
    DevToolApp& operator=(const DevToolApp&) = delete;

public:
    void show() const;

    void hide() const;

    bool visible() const;

    void appendError(std::string msg);

    // 注册菜单; 注册时注入 WindowManager 并触发 onAttached(元素自注册窗口, 须早于 render 线程启动)
    template <typename T, typename... Args>
        requires std::derived_from<T, IMenu> && std::is_final_v<T>
    T& registerMenu(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        return static_cast<T&>(this->registerMenu(std::move(ptr)));
    }

    IMenu& registerMenu(std::unique_ptr<IMenu> menu);

    [[nodiscard]] static std::unique_ptr<DevToolApp> make();
};


} // namespace devtool
