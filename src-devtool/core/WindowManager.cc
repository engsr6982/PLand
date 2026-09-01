#include "core/WindowManager.h"
#include "pland/PLand.h"
#include <algorithm>
#include <cassert>
#include <imgui_internal.h>
#include <utility>

namespace devtool {

namespace {

ImGuiDir toImGuiDir(DockDirection dir) {
    switch (dir) {
    case DockDirection::Left:
        return ImGuiDir_Left;
    case DockDirection::Right:
        return ImGuiDir_Right;
    case DockDirection::Up:
        return ImGuiDir_Up;
    case DockDirection::Down:
        return ImGuiDir_Down;
    default:
        return ImGuiDir_None;
    }
}

} // namespace

IWindow& WindowManager::registerOwnedImpl(std::unique_ptr<IWindow> window) {
    auto* ptr = window.get();
    if (!byTitle_.emplace(ptr->title(), ptr).second) {
        // ImGui 以标题作为窗口标识, 重复标题会破坏停靠/ini 恢复
        land::PLand::getInstance().getSelf().getLogger().error(
            "WindowManager: duplicate window title \"{}\"",
            ptr->title()
        );
        assert(false && "duplicate window title");
    }
    windows_.push_back(std::move(window));
    return *ptr;
}

void WindowManager::unregister(IWindow* window) {
    auto it =
        std::find_if(windows_.begin(), windows_.end(), [window](auto const& owned) { return owned.get() == window; });
    if (it == windows_.end()) {
        return;
    }
    byTitle_.erase(window->title());
    pendingDestroy_.push_back(std::move(*it)); // 帧末析构, 绝不在窗口 Begin/End 之间销毁
    windows_.erase(it);
}

IWindow* WindowManager::find(std::string_view title) const {
    auto it = byTitle_.find(std::string(title));
    return it == byTitle_.end() ? nullptr : it->second;
}

void WindowManager::frameBegin(ImGuiID dockspaceId) {
    dockspaceId_ = dockspaceId;
    ImGui::DockSpaceOverViewport(dockspaceId_, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    // 布局不持久化: 每 session 首次帧按 DockSpec 构建默认布局
    if (!layoutInitialized_) {
        layoutInitialized_ = true;
        buildLayoutFromSpecs();
    }
}

void WindowManager::renderAll() {
    for (size_t i = 0; i < windows_.size(); ++i) { // 允许循环中注册新窗口(动态窗口)
        IWindow* window = windows_[i].get();
        if (!window->enabled()) {
            ImGui::BeginDisabled(true);
        }
        window->render();
        if (!window->enabled()) {
            ImGui::EndDisabled();
        }
    }
}

void WindowManager::processDockRequests() {
    for (auto it = pendingDocks_.begin(); it != pendingDocks_.end();) {
        // 目标/宿主已注销, 放弃
        if (byTitle_.find(it->host->title()) == byTitle_.end()
            || byTitle_.find(it->target->title()) == byTitle_.end()) {
            it = pendingDocks_.erase(it);
            continue;
        }
        ImGuiID hostDock   = it->host->dockId();
        ImGuiID targetDock = it->target->dockId();
        if (hostDock == 0) {
            // 宿主悬浮, 放弃(目标留在主停靠区)
            it = pendingDocks_.erase(it);
            continue;
        }
        if (targetDock == 0) {
            // 目标尚未 Begin 过(本帧创建), 下帧重试
            if (++it->attempts > 60) {
                it = pendingDocks_.erase(it);
            } else {
                ++it;
            }
            continue;
        }
        if (targetDock != hostDock) {
            // DockBuilder 调用只允许出现在帧内所有 Begin/End 之后(防父子停靠卡死)
            ImGui::DockBuilderDockWindow(it->target->title().data(), hostDock);
            ImGui::DockBuilderFinish(hostDock);
        }
        it = pendingDocks_.erase(it);
    }
}

void WindowManager::postFrame() {
    pendingDestroy_.clear(); // 析构延迟销毁的窗口(render 线程, 帧末)
}

void WindowManager::requestDockInto(IWindow* host, IWindow* target) { pendingDocks_.push_back({host, target, 0}); }

void WindowManager::buildLayoutFromSpecs() {
    // 经典 wiki 配方: AddNode(id) 内部先 RemoveNode, 保证幂等
    ImGuiID root = ImGui::DockBuilderAddNode(dockspaceId_, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(root, ImGui::GetMainViewport()->WorkSize);

    // 注意: DockSpec::parent 必须是注册顺序中更早的窗口(或 "")。
    // 分栏语义: 节点内已有窗口留在 opposite 侧, at_dir 侧为空。
    std::unordered_map<std::string, ImGuiID> nodeOf;
    nodeOf.emplace("", root);
    for (auto const& owned : windows_) {
        auto const& spec = owned->dockSpec();
        if (spec.direction == DockDirection::None) {
            continue; // 悬浮窗口
        }
        auto it = nodeOf.find(std::string(spec.parent));
        if (it == nodeOf.end()) {
            continue; // 宿主节点不存在, 跳过(窗口将停进主停靠区)
        }
        if (spec.direction == DockDirection::Tab) {
            ImGui::DockBuilderDockWindow(owned->title().c_str(), it->second);
        } else {
            ImGuiID atDir, atOpp;
            ImGui::DockBuilderSplitNode(it->second, toImGuiDir(spec.direction), spec.sizeRatio, &atDir, &atOpp);
            nodeOf[owned->title()]           = atDir;
            nodeOf[std::string(spec.parent)] = atOpp;
            ImGui::DockBuilderDockWindow(owned->title().c_str(), atDir);
        }
    }
    ImGui::DockBuilderFinish(dockspaceId_);
}

void WindowManager::resetLayout() { buildLayoutFromSpecs(); }

} // namespace devtool
