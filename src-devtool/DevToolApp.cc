// ImGui 1.92+ 的 ImVec2 数学运算符默认禁用, 需在包含 imgui.h 之前定义
#define IMGUI_DEFINE_MATH_OPERATORS
#include "DevToolApp.h"
#include "core/WindowManager.h"
#include "menus/window/WindowMenu.h"
#include "pland/PLand.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <iterator>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

namespace devtool {

namespace {

// 校验 ini 是否包含有效停靠布局(防止空 ini 吞掉首次布局)
bool fileContains(std::filesystem::path const& path, std::string_view needle) {
    std::ifstream ifs(path);
    if (!ifs) {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return content.find(needle) != std::string::npos;
}

// BDS 主线程 -> render 线程的轻量命令队列
struct CommandQueue {
    std::mutex                         mutex;
    std::vector<std::function<void()>> queue;

    void push(std::function<void()> fn) {
        std::lock_guard lock(mutex);
        queue.emplace_back(std::move(fn));
    }
    void drain() {
        std::vector<std::function<void()>> batch;
        {
            std::lock_guard lock(mutex);
            batch.swap(queue);
        }
        for (auto& fn : batch) {
            fn();
        }
    }
};

} // namespace

struct DevToolApp::Impl {
    GLFWwindow*                         glfwWindow_{nullptr};
    std::thread                         renderThread_;
    std::atomic<bool>                   renderThreadStopFlag_{false};
    std::atomic<bool>                   visible_{false}; // BDS 线程无锁读
    CommandQueue                        commands_;
    std::vector<std::string>            errors_; // 仅 render 线程
    std::unique_ptr<WindowManager>      windowManager_;
    std::vector<std::unique_ptr<IMenu>> menus_; // 注册后仅 render 线程读
    int                                 styleIndex_{1};
    float                               prevScale_{0.0f};
    float                               appliedScale_{1.0f};
    ImGuiID                             dockspaceId_{0};
    std::string                         iniFilename_; // io.IniFilename 指向的字符串, 须长期存活
    std::filesystem::path               configDir_;
    std::filesystem::path               iniPath_;

    static void _handleGlfwError(int error, const char* description) {
        land::PLand::getInstance().getSelf().getLogger().error("GLFW Error: {}: {}", error, description);
    }

    static void _handleWindowClose(GLFWwindow* window) {
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        glfwHideWindow(window);
    }

    bool _initImGuiAndOpenGlWithGLFW() {
        if (glfwWindow_) {
            throw std::runtime_error("GLFW window already initialized");
        }

        glfwSetErrorCallback(_handleGlfwError);
        if (!glfwInit()) {
            land::PLand::getInstance().getSelf().getLogger().error("Failed to initialize GLFW");
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 设置OpenGL版本
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // 设置窗口默认是否可见

        {
            float        xScale, yScale;
            GLFWmonitor* monitor = glfwGetPrimaryMonitor(); // 获取主显示器
            if (!monitor) {
                land::PLand::getInstance().getSelf().getLogger().error("Failed to get primary monitor");
                glfwTerminate();
                return false;
            }
            glfwGetMonitorContentScale(monitor, &xScale, &yScale); // 获取主显示器缩放比例
            this->glfwWindow_ = glfwCreateWindow(
                static_cast<int>(900 * xScale),
                static_cast<int>(560 * yScale),
                "PLand - DevTools",
                nullptr,
                nullptr
            );
            if (!this->glfwWindow_) {
                land::PLand::getInstance().getSelf().getLogger().error("Failed to create GLFW window");
                glfwTerminate();
                return false;
            }

            glfwSetWindowCloseCallback(this->glfwWindow_, _handleWindowClose); // 设置窗口关闭回调函数
            glfwMakeContextCurrent(this->glfwWindow_);                         // 设置当前上下文
            glfwSwapInterval(1);                                               // 设置垂直同步

            if (glewInit() != GLEW_OK) {
                land::PLand::getInstance().getSelf().getLogger().error("Failed to initialize GLEW");

                glfwDestroyWindow(this->glfwWindow_);
                glfwTerminate(); // 终止GLFW
                this->glfwWindow_ = nullptr;
                return false;
            }
        }

        // 初始化ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io     = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 启用键盘导航
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // 启用Docking
        io.IniFilename  = this->iniFilename_.c_str();         // 停靠布局持久化
        ImGui::StyleColorsDark();

        // 初始化ImGui与GLFW的绑定
        ImGui_ImplGlfw_InitForOpenGL(this->glfwWindow_, true); // ImGui <> GLFW
        ImGui_ImplOpenGL3_Init("#version 130");                // ImGui <> OpenGL
        return true;
    }

    void _checkAndUpdateScale() {
        float xScale;
        float yScale;
        glfwGetWindowContentScale(this->glfwWindow_, &xScale, &yScale);
        if (this->prevScale_ == xScale) {
            return;
        }
        this->prevScale_ = xScale;

        ImGuiIO& io = ImGui::GetIO();

        io.Fonts->Clear();

        // 加载 Consolas 字体避免 msyh.ttc 的非等宽字符导致显示问题
        ImFontConfig config;
        config.SizePixels = std::round(16 * xScale);

        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", config.SizePixels, &config);

        config.MergeMode = true; // 启用合并模式

        io.Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\msyh.ttc", // 借用微软雅黑
            config.SizePixels,
            &config,
            io.Fonts->GetGlyphRangesChineseFull() // 只合并中文/全角标点集合
        );

        io.Fonts->Build();

        // Since 1.92, the OpenGL3 backend handles font texture (re)creation automatically
        // via the ImTextureData request protocol; ImGui_ImplOpenGL3_CreateFontsTexture()
        // and ImGui_ImplOpenGL3_DestroyFontsTexture() were removed from the public API.

        // 按增量缩放样式(ScaleAllSizes 是乘性缩放, 不能直接用当前比例)
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(xScale / this->appliedScale_);
        this->appliedScale_ = xScale;
    }

    void _render() {
        if (!_initImGuiAndOpenGlWithGLFW()) {
            land::PLand::getInstance().getSelf().getLogger().error(
                "DevTools failed to initialize, render thread exited (may happen when the display is off)"
            );
            return;
        }

        while (!this->renderThreadStopFlag_.load()) {
            this->commands_.drain(); // 跨线程命令
            glfwPollEvents();

            // 检查窗口是否可见
            if (!glfwGetWindowAttrib(this->glfwWindow_, GLFW_VISIBLE)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue; // 窗口不可见时跳过渲染
            }

            _checkAndUpdateScale(); // 检查并更新缩放比例

            // 创建新的渲染帧
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // ImGui::GetID 受当前窗口 ID 栈影响, 必须在任何 Begin 之前计算停靠区 ID
            if (this->dockspaceId_ == 0) {
                this->dockspaceId_ = ImGui::GetID("PLandDockspace");
            }

            // 自上而下的渲染管线: 菜单栏 -> 停靠区 -> 平铺窗口 -> 停靠请求 -> 浮层
            _renderMainMenuBar();
            this->windowManager_->frameBegin(this->dockspaceId_);
            this->windowManager_->renderAll();
            this->windowManager_->processDockRequests();
            _renderErrorsOverlay();
            this->windowManager_->postFrame();

            // 渲染帧
            ImGui::Render();
            int displayW;
            int displayH;
            glfwGetFramebufferSize(this->glfwWindow_, &displayW, &displayH);
            if (displayW > 0 && displayH > 0) { // 零尺寸防护(窗口最小化)
                glViewport(0, 0, displayW, displayH);
                glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(this->glfwWindow_);
            }
        }

        // 线程退出: 销毁 ImGui 上下文
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(); // 自动写 imgui.ini
        glfwDestroyWindow(this->glfwWindow_);
        this->glfwWindow_ = nullptr;
        glfwTerminate();
    }

    void _renderMainMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            _renderAppOptionsMenu();
            for (auto& menu : menus_) {
                menu->render();
            }
            _renderFramerateInfo();
            ImGui::EndMainMenuBar();
        }
    }

    void _renderAppOptionsMenu() {
        if (ImGui::BeginMenu("选项")) {
            ImGui::SetNextItemWidth(100);
            if (ImGui::Combo("主题", &this->styleIndex_, "Light\0Dark\0Classic\0")) {
                this->_applyTheme(this->styleIndex_);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("重置布局")) {
                this->windowManager_->resetLayout();
            }
            ImGui::EndMenu();
        }
    }

    void _renderFramerateInfo() const {
        auto const& frame = ImGui::GetIO().Framerate;
        auto        text  = fmt::format("{:.1f}ms/frame ({:.1f} FPS)", 1000.0f / frame, frame);

        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth   = ImGui::CalcTextSize(text.c_str()).x;
        ImGui::SetCursorPosX(windowWidth - textWidth);
        ImGui::Text("%s", text.c_str());
    }

    void _renderErrorsOverlay() {
        if (this->errors_.empty()) {
            return;
        }
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos + ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.85F);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration
                               | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
                               | ImGuiWindowFlags_NoFocusOnAppearing;
        if (ImGui::Begin("##DevToolErrors", nullptr, flags)) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 40, 255));
            for (auto const& error : this->errors_) {
                ImGui::TextUnformatted(error.data());
            }
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }

    void _applyTheme(int index) {
        switch (index) {
        case 0:
            ImGui::StyleColorsLight();
            break;
        case 2:
            ImGui::StyleColorsClassic();
            break;
        default:
            ImGui::StyleColorsDark();
            break;
        }
    }

    void initialize() {
        this->renderThread_ = std::thread(&Impl::_render, this); // 创建渲染线程
    }

    void shutdown() {
        this->renderThreadStopFlag_.store(true);
        if (this->renderThread_.joinable()) {
            this->renderThread_.join(); // join 返回前配置与 ini 均已落盘
        }
    }

    void show() {
        this->visible_.store(true); // 立即反映意图
        this->commands_.push([this] {
            if (this->glfwWindow_) {
                glfwShowWindow(this->glfwWindow_);
            } else {
                land::PLand::getInstance().getSelf().getLogger().error("DevTools window not initialized");
            }
        });
    }

    void hide() {
        this->visible_.store(false);
        this->commands_.push([this] {
            if (this->glfwWindow_) glfwHideWindow(this->glfwWindow_);
        });
    }

    bool visible() const { return this->visible_.load(); }

    void appendError(std::string msg) {
        this->commands_.push([this, msg = std::move(msg)]() mutable {
            if (this->errors_.size() >= 50) {
                this->errors_.erase(this->errors_.begin());
            }
            this->errors_.emplace_back(std::move(msg));
        });
    }
};


DevToolApp::DevToolApp() : impl(std::make_unique<Impl>()) {}

DevToolApp::~DevToolApp() { impl->shutdown(); }
void DevToolApp::show() const { impl->show(); }
void DevToolApp::hide() const { impl->hide(); }
bool DevToolApp::visible() const { return impl->visible(); }
void DevToolApp::appendError(std::string msg) { impl->appendError(std::move(msg)); }

IMenu& DevToolApp::registerMenu(std::unique_ptr<IMenu> menu) {
    // 注入 WindowManager 并触发菜单初始化: 元素在 onAttached 中自注册各自窗口(须早于 render 线程启动)
    if (!impl->windowManager_) {
        land::PLand::getInstance().getSelf().getLogger().error(
            "DevToolApp: WindowManager is not initialized, menu \"{}\" has no windows",
            menu->getLabel()
        );
        assert(false && "DevToolApp::registerMenu before WindowManager initialization");
    } else {
        menu->onAttached(*impl->windowManager_);
    }
    return *impl->menus_.emplace_back(std::move(menu));
}

std::unique_ptr<DevToolApp> DevToolApp::make() {
    auto  app_ = std::make_unique<DevToolApp>();
    auto& impl = *app_->impl;

    impl.configDir_ = land::PLand::getInstance().getSelf().getConfigDir();
    std::filesystem::create_directories(impl.configDir_);
    impl.iniPath_     = impl.configDir_ / "devtool_imgui.ini";
    impl.iniFilename_ = impl.iniPath_.string();

    // 仅有有效停靠布局的 ini 才允许直接恢复; 空 ini(窗口从未显示时由 DestroyContext 写出)走默认布局
    bool iniValid       = std::filesystem::exists(impl.iniPath_) && fileContains(impl.iniPath_, "[Docking][Data]");
    impl.windowManager_ = std::make_unique<WindowManager>(impl.iniPath_, iniValid);

    app_->registerMenu<WindowMenu>();

    impl.initialize();
    return app_;
}

} // namespace devtool
