#include "DevToolApp.h"
#include "pland/mod/PLand.h"
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

namespace devtool {

DevToolApp::DevToolApp() {
    this->renderThread_ = std::thread(&DevToolApp::render, this); // 创建渲染线程
}

DevToolApp::~DevToolApp() {
    this->renderThreadStopFlag_.store(true);
    if (this->renderThread_.joinable()) {
        this->renderThread_.join();
    }
}

void DevToolApp::show() const {
    if (glfwWindow_) {
        glfwShowWindow(glfwWindow_);
    } else {
        mod::PLand::getInstance().getSelf().getLogger().error("DevTools window not initialized");
    }
}

void DevToolApp::hide() const {
    if (glfwWindow_) glfwHideWindow(glfwWindow_);
}

void DevToolApp::appendError(std::string msg) { this->errors_.emplace_back(std::move(msg)); }


void DevToolApp::render() {
    initImGuiAndOpenGlWithGLFW();

    while (!this->renderThreadStopFlag_.load()) {
        glfwPollEvents();

        // 检查窗口是否可见
        if (!glfwGetWindowAttrib(this->glfwWindow_, GLFW_VISIBLE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue; // 窗口不可见时跳过渲染
        }

        checkAndUpdateScale(); // 检查并更新缩放比例

        // 创建新的渲染帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 渲染组件
        renderErrors();
        renderMainMenuBar();
        postTick();

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

void DevToolApp::checkAndUpdateScale() {
    static float prevScale = 0;
    float        xScale, yScale;
    glfwGetWindowContentScale(this->glfwWindow_, &xScale, &yScale);
    if (prevScale != xScale) {
        prevScale   = xScale;
        ImGuiIO& io = ImGui::GetIO();

        auto fontPath = mod::PLand::getInstance().getSelf().getDataDir() / "fonts" / "font.ttf";
        if (!std::filesystem::exists(fontPath)) {
            this->appendError(fmt::format(
                "由于字体文件 ( {} ) 不存在\n这可能导致部分模块字体显示异常\n\n建议下载 maple-font 字体的 "
                "Normal-Ligature CN 版本\n将 \"MapleMonoNormal-CN-Regular.ttf\" 重命名为 font.ttf 放置在上述路径下",
                fontPath.string()
            ));
            fontPath = "C:/Windows/Fonts/msyh.ttc";
        }

        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF(
            fontPath.string().c_str(),
            std::round(15 * xScale),
            nullptr,
            io.Fonts->GetGlyphRangesChineseFull()
        );
        io.Fonts->Build();
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
        auto style = ImGuiStyle();
        style.ScaleAllSizes(xScale);
    }
}

void DevToolApp::initImGuiAndOpenGlWithGLFW() {
    if (glfwWindow_) {
        throw std::runtime_error("GLFW window already initialized");
    }

    glfwSetErrorCallback(internals::handleGlfwError);
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW！");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 设置OpenGL版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // 设置窗口默认是否可见
#ifdef DEBUG
    // glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#else
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#endif

    {
        float        xScale, yScale;
        GLFWmonitor* monitor = glfwGetPrimaryMonitor(); // 获取主显示器
        if (!monitor) {
            mod::PLand::getInstance().getSelf().getLogger().error("Failed to get primary monitor");
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
            mod::PLand::getInstance().getSelf().getLogger().error("Failed to create GLFW window");
            return;
        }

        glfwSetWindowCloseCallback(this->glfwWindow_, internals::handleWindowClose); // 设置窗口关闭回调函数
        glfwMakeContextCurrent(this->glfwWindow_);                                   // 设置当前上下文
        glfwSwapInterval(1);                                                         // 设置垂直同步

        if (glewInit() != GLEW_OK) {
            mod::PLand::getInstance().getSelf().getLogger().error("Failed to initialize GLEW");

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
    ImGui::StyleColorsDark();

    // 初始化ImGui与GLFW的绑定
    ImGui_ImplGlfw_InitForOpenGL(this->glfwWindow_, true); // ImGui <> GLFW
    ImGui_ImplOpenGL3_Init("#version 130");                // ImGui <> OpenGL
}


void DevToolApp::renderErrors() const {
    if (!this->errors_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 40, 255));
        for (auto& warning : this->errors_) {
            ImGui::TextUnformatted(warning.data());
        }
        ImGui::PopStyleColor();
    }
}

void DevToolApp::renderMainMenuBar() const {
    auto                  dockspaceId = ImGui::DockSpaceOverViewport();
    static std::once_flag firstTime;
    std::call_once(firstTime, [&] {
        for (auto& menu : this->menus_) {
            ImGui::DockBuilderDockWindow(menu->getLabel().data(), dockspaceId);
        }
        ImGui::DockBuilderFinish(dockspaceId);
    });

    if (ImGui::BeginMainMenuBar()) {
        renderAppOptionsMenu();
        for (auto& menu : menus_) {
            menu->render();
        }
        renderFramerateInfo();
        ImGui::EndMainMenuBar();
    }
}

void DevToolApp::renderFramerateInfo() const {
    auto const& frame = ImGui::GetIO().Framerate;
    auto        text  = fmt::format("{:.1f}ms/frame ({:.1f} FPS)", 1000.0f / frame, frame);

    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth   = ImGui::CalcTextSize(text.c_str()).x;
    ImGui::SetCursorPosX(windowWidth - textWidth);
    ImGui::Text(text.c_str());
}

void DevToolApp::renderAppOptionsMenu() const {
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
        if (ImGui::Button("关闭(隐藏)")) {
            hide();
        }
        ImGui::EndMenu();
    }
}

void DevToolApp::postTick() const {
    for (auto& menu : menus_) {
        menu->tick();
    }
}


namespace internals {

void handleGlfwError(int error, const char* description) {
    mod::PLand::getInstance().getSelf().getLogger().error("GLFW Error: {}: {}", error, description);
}

void handleWindowClose(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    glfwHideWindow(window);
}

} // namespace internals

} // namespace devtool