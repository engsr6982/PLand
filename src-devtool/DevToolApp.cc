#include "DevToolApp.h"
#include "pland/PLand.h"
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <memory>

#include "menus/helper/HelperMenu.h"
#include "menus/viewer/ViewerMenu.h"

namespace devtool {

struct DevToolApp::Impl {
    GLFWwindow*                         glfwWindow_{nullptr};
    std::thread                         renderThread_;
    std::atomic<bool>                   renderThreadStopFlag_;
    std::vector<std::string>            errors_;
    std::vector<std::unique_ptr<IMenu>> menus_;

    static void _handleGlfwError(int error, const char* description) {
        land::PLand::getInstance().getSelf().getLogger().error("GLFW Error: {}: {}", error, description);
    }

    static void _handleWindowClose(GLFWwindow* window) {
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        glfwHideWindow(window);
    }

    void _initImGuiAndOpenGlWithGLFW() {
        if (glfwWindow_) {
            throw std::runtime_error("GLFW window already initialized");
        }

        glfwSetErrorCallback(_handleGlfwError);
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW！");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 设置OpenGL版本
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // 设置窗口默认是否可见

        {
            float        xScale, yScale;
            GLFWmonitor* monitor = glfwGetPrimaryMonitor(); // 获取主显示器
            if (!monitor) {
                land::PLand::getInstance().getSelf().getLogger().error("Failed to get primary monitor");
                return;
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
                return;
            }

            glfwSetWindowCloseCallback(this->glfwWindow_, _handleWindowClose); // 设置窗口关闭回调函数
            glfwMakeContextCurrent(this->glfwWindow_);                         // 设置当前上下文
            glfwSwapInterval(1);                                               // 设置垂直同步

            if (glewInit() != GLEW_OK) {
                land::PLand::getInstance().getSelf().getLogger().error("Failed to initialize GLEW");

                glfwDestroyWindow(this->glfwWindow_);
                glfwTerminate(); // 终止GLFW
                this->glfwWindow_ = nullptr;
                return;
            }
        }

        // 初始化ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io     = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 启用键盘导航
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // 启用Docking
        io.IniFilename  = nullptr;                            // 删除配置文件
        ImGui::StyleColorsDark();

        // 初始化ImGui与GLFW的绑定
        ImGui_ImplGlfw_InitForOpenGL(this->glfwWindow_, true); // ImGui <> GLFW
        ImGui_ImplOpenGL3_Init("#version 130");                // ImGui <> OpenGL
    }

    void _checkAndUpdateScale() {
        static float prevScale = 0;
        float        xScale, yScale;
        glfwGetWindowContentScale(this->glfwWindow_, &xScale, &yScale);
        if (prevScale != xScale) {
            prevScale   = xScale;
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

            auto style = ImGuiStyle();
            style.ScaleAllSizes(xScale);
        }
    }

    void _render() {
        _initImGuiAndOpenGlWithGLFW();

        while (!this->renderThreadStopFlag_.load()) {
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

            // 渲染组件
            _renderErrors();
            _renderMainMenuBar();
            _postTick();

            // 渲染帧
            ImGui::Render();
            int displayW;
            int displayH;
            glfwGetFramebufferSize(this->glfwWindow_, &displayW, &displayH);
            glViewport(0, 0, displayW, displayH);
            glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(this->glfwWindow_);
        }

        // 线程退出销毁资源
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(this->glfwWindow_);
        this->glfwWindow_ = nullptr;
        glfwTerminate();
    }

    void _renderErrors() const {
        if (!this->errors_.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 40, 255));
            for (auto& warning : this->errors_) {
                ImGui::TextUnformatted(warning.data());
            }
            ImGui::PopStyleColor();
        }
    }

    void _renderMainMenuBar() const {
        auto                  dockspaceId = ImGui::DockSpaceOverViewport();
        static std::once_flag firstTime;
        std::call_once(firstTime, [&] {
            for (auto& menu : this->menus_) {
                ImGui::DockBuilderDockWindow(menu->getLabel().data(), dockspaceId);
            }
            ImGui::DockBuilderFinish(dockspaceId);
        });

        if (ImGui::BeginMainMenuBar()) {
            _renderAppOptionsMenu();
            for (auto& menu : menus_) {
                menu->render();
            }
            _renderFramerateInfo();
            ImGui::EndMainMenuBar();
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

    void _renderAppOptionsMenu() const {
        if (ImGui::BeginMenu("选项")) {
            {
                static int styleIndex = -1;
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo("主题", &styleIndex, "Light\0Dark\0Classic\0")) {
                    switch (styleIndex) {
                    case 0:
                        ImGui::StyleColorsLight();
                        break;
                    case 1:
                        ImGui::StyleColorsDark();
                        break;
                    case 2:
                        ImGui::StyleColorsClassic();
                        break;
                    default:
                        break;
                    }
                }
            }
            ImGui::EndMenu();
        }
    }

    void _postTick() const {
        for (auto& menu : menus_) {
            menu->tick();
        }
    }


    void initialize() {
        this->renderThread_ = std::thread(&Impl::_render, this); // 创建渲染线程
    }

    void shutdown() {
        this->renderThreadStopFlag_.store(true);
        if (this->renderThread_.joinable()) {
            this->renderThread_.join();
        }
    }

    void show() const {
        if (glfwWindow_) {
            glfwShowWindow(glfwWindow_);
        } else {
            land::PLand::getInstance().getSelf().getLogger().error("DevTools window not initialized");
        }
    }

    void hide() const {
        if (glfwWindow_) glfwHideWindow(glfwWindow_);
    }

    bool visible() const {
        if (glfwWindow_) return glfwGetWindowAttrib(glfwWindow_, GLFW_VISIBLE) != 0;
        return false;
    }

    void appendError(std::string msg) { this->errors_.emplace_back(std::move(msg)); }
};


DevToolApp::DevToolApp() : impl(std::make_unique<Impl>()) { impl->initialize(); }

DevToolApp::~DevToolApp() { impl->shutdown(); }
void DevToolApp::show() const { impl->show(); }
void DevToolApp::hide() const { impl->hide(); }
bool DevToolApp::visible() const { return impl->visible(); }
void DevToolApp::appendError(std::string msg) { impl->appendError(std::move(msg)); }

void DevToolApp::registerMenu(std::unique_ptr<IMenu> menu) { impl->menus_.emplace_back(std::move(menu)); }

std::unique_ptr<DevToolApp> DevToolApp::make() {
    auto app_ = std::make_unique<DevToolApp>();
    app_->registerMenu<viewer::ViewerMenu>();
    app_->registerMenu<helper::HelperMenu>();
    return std::move(app_);
}

} // namespace devtool