#include "LandTreeViewer.h"
#include "fmt/compile.h"
#include "pland/PLand.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/service/LandHierarchyService.h"
#include "pland/service/ServiceLocator.h"
#include <algorithm>
#include <fmt/core.h>
#include <utility>


namespace devtool::viewer {

// --- 配置常量 ---
static const float NODE_W  = 180.0f;
static const float NODE_H  = 90.0f;
static const float SPACE_X = 30.0f;
static const float SPACE_Y = 120.0f;

// 颜色定义
static const ImU32 C_BG     = IM_COL32(32, 32, 32, 255);
static const ImU32 C_GRID   = IM_COL32(200, 200, 200, 40);
static const ImU32 C_LINK   = IM_COL32(255, 255, 255, 180);
static const ImU32 C_ORD    = IM_COL32(100, 200, 100, 255); // Green
static const ImU32 C_PARENT = IM_COL32(200, 100, 100, 255); // Red
static const ImU32 C_MIX    = IM_COL32(200, 200, 100, 255); // Yellow
static const ImU32 C_SUB    = IM_COL32(100, 150, 250, 255); // Blue


LandTreeViewer::LandTreeViewer(std::string title, land::LandID rootId) : IWindow(std::move(title)), mRootId(rootId) {}

void LandTreeViewer::render() {
    WindowScope scope(*this);
    if (!scope.isOpen()) {
        return;
    }
    ImGui::Text("Viewing Hierarchy Root: %lld", mRootId);

    auto& locator          = land::PLand::getInstance().getServiceLocator();
    auto& hierarchyService = locator.getLandHierarchyService();
    auto& registry         = land::PLand::getInstance().getLandRegistry();

    // 获取根领地对象
    std::shared_ptr<land::Land> rootLand = registry.getLand(mRootId);
    if (!rootLand) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid Land ID or Land not found.");
        return;
    }

    // 构建树结构 (即时模式)
    mLayoutRoot = _buildTree(rootLand, hierarchyService);
    if (!mLayoutRoot) return;

    // 计算布局
    _calculateSubtreeWidth(mLayoutRoot.get());
    _assignCoordinates(mLayoutRoot.get(), 0.0f, 0.0f);

    // 处理画布输入
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    // 绘制背景
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvas_p0, canvas_p1, C_BG);
    dl->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 40));

    // 隐形按钮捕获鼠标交互
    ImGui::InvisibleButton(
        "##node_tree_canvas",
        canvas_sz,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight
    );
    const bool is_hovered = ImGui::IsItemHovered();
    const bool is_active  = ImGui::IsItemActive();

    // Pan (右键拖拽)
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
        mScrolling.x += ImGui::GetIO().MouseDelta.x;
        mScrolling.y += ImGui::GetIO().MouseDelta.y;
    }

    // Zoom (滚轮)
    if (is_hovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float old_zoom  = mZoom;
            mZoom          += wheel * 0.1f;
            mZoom           = std::clamp(mZoom, 0.1f, 3.0f);

            // 以鼠标为中心缩放
            ImVec2 mouse_rel = ImVec2(ImGui::GetIO().MousePos.x - canvas_p0.x, ImGui::GetIO().MousePos.y - canvas_p0.y);
            // new_scrolling = mouse - (mouse - old_scrolling) * (new_zoom / old_zoom)
            // 推导：WorldPos 不变，WorldPos = (Screen - Scroll) / Zoom
            ImVec2 world_pos = ImVec2((mouse_rel.x - mScrolling.x) / old_zoom, (mouse_rel.y - mScrolling.y) / old_zoom);
            mScrolling.x     = mouse_rel.x - world_pos.x * mZoom;
            mScrolling.y     = mouse_rel.y - world_pos.y * mZoom;
        }
    }

    // 绘制
    dl->PushClipRect(canvas_p0, canvas_p1, true);

    ImVec2 origin = ImVec2(canvas_p0.x + mScrolling.x, canvas_p0.y + mScrolling.y);

    // 递归绘制连线 (后序遍历，底层)
    std::vector<NodeLayout*> queue;
    queue.push_back(mLayoutRoot.get());
    size_t head = 0;
    while (head < queue.size()) {
        auto* curr = queue[head++];
        for (auto& child : curr->children) {
            _drawLink(dl, curr, child.get(), origin, mZoom);
            queue.push_back(child.get());
        }
    }

    // 递归绘制节点 (顶层)
    head = 0; // reset queue logic or just reuse
    queue.clear();
    queue.push_back(mLayoutRoot.get());
    head = 0;
    while (head < queue.size()) {
        auto* curr = queue[head++];
        _drawNode(dl, curr, origin, mZoom);
        for (auto& child : curr->children) {
            queue.push_back(child.get());
        }
    }

    dl->PopClipRect();
}

std::unique_ptr<LandTreeViewer::NodeLayout>
LandTreeViewer::_buildTree(std::shared_ptr<land::Land> const& root, land::service::LandHierarchyService& service) {
    auto node   = std::make_unique<NodeLayout>();
    node->land  = root;
    node->width = 0;
    node->x     = 0;
    node->y     = 0;

    if (root->hasSubLand()) {
        auto subLands = service.getSubLands(root);
        node->children.reserve(subLands.size());
        for (auto& sub : subLands) {
            // 递归构建
            node->children.push_back(_buildTree(sub, service));
        }
    }
    return node;
}

float LandTreeViewer::_calculateSubtreeWidth(NodeLayout* node) {
    if (node->children.empty()) {
        node->width = NODE_W;
        return NODE_W;
    }

    float totalW = 0.0f;
    for (auto& child : node->children) {
        totalW += _calculateSubtreeWidth(child.get());
    }
    // 加上间隙
    totalW += (node->children.size() - 1) * SPACE_X;

    node->width = std::max(NODE_W, totalW);
    return node->width;
}

void LandTreeViewer::_assignCoordinates(NodeLayout* node, float x_offset, float y_depth) {
    node->y = y_depth;

    if (node->children.empty()) {
        node->x = x_offset + NODE_W / 2.0f;
    } else {
        float childX = x_offset;
        // 如果该节点比其子树窄，需要居中对齐子树
        // 实际上 _calculateSubtreeWidth 保证了 node->width >= subTreeWidth
        // 这里我们需要把子树紧凑排列在 x_offset 区域内

        for (auto& child : node->children) {
            _assignCoordinates(child.get(), childX, y_depth + NODE_H + SPACE_Y);
            childX += child->width + SPACE_X;
        }

        // 父节点位于首尾子节点的中心
        float firstCenter = node->children.front()->x;
        float lastCenter  = node->children.back()->x;
        node->x           = (firstCenter + lastCenter) / 2.0f;
    }
}

void LandTreeViewer::_drawLink(
    ImDrawList*   dl,
    NodeLayout*   parent,
    NodeLayout*   child,
    const ImVec2& origin,
    float         scale
) {
    ImVec2 p1 = ImVec2(origin.x + parent->x * scale, origin.y + (parent->y + NODE_H) * scale); // Parent Bottom
    ImVec2 p2 = ImVec2(origin.x + child->x * scale, origin.y + child->y * scale);              // Child Top

    ImVec2 cp1 = ImVec2(p1.x, p1.y + 50.0f * scale);
    ImVec2 cp2 = ImVec2(p2.x, p2.y - 50.0f * scale);

    dl->AddBezierCubic(p1, cp1, cp2, p2, C_LINK, 2.0f * scale);
}

void LandTreeViewer::_drawNode(ImDrawList* dl, NodeLayout* node, const ImVec2& origin, float scale) {
    float w = NODE_W * scale;
    float h = NODE_H * scale;
    float x = origin.x + node->x * scale - (w / 2.0f); // 转换中心坐标到左上角
    float y = origin.y + node->y * scale;

    ImVec2 p_min(x, y);
    ImVec2 p_max(x + w, y + h);

    ImU32 color = C_ORD;
    if (node->land->isParentLand()) color = C_PARENT;
    else if (node->land->isMixLand()) color = C_MIX;
    else if (node->land->isSubLand()) color = C_SUB;

    // 背景 & 边框
    dl->AddRectFilled(p_min, p_max, color, 6.0f * scale);
    dl->AddRect(p_min, p_max, IM_COL32_WHITE, 6.0f * scale, 0, 1.5f * scale);

    // 文本 (如果缩放太小就不显示)
    if (scale > 0.3f) {
        auto        land  = node->land;
        std::string line1 = fmt::format("[{}] {}", land->getId(), land->getName());
        std::string line2 = fmt::format("Lvl: {}", land->getNestedLevel());
        std::string line3 = fmt::format("Dim: {}", land->getDimensionId());

        ImVec2 txtPos = ImVec2(x + 5 * scale, y + 5 * scale);
        dl->AddText(NULL, 14.0f * scale, txtPos, IM_COL32_WHITE, line1.c_str());

        txtPos.y += 16.0f * scale;
        dl->AddText(NULL, 12.0f * scale, txtPos, IM_COL32(220, 220, 220, 255), line2.c_str());

        txtPos.y += 14.0f * scale;
        dl->AddText(NULL, 12.0f * scale, txtPos, IM_COL32(200, 200, 200, 255), line3.c_str());
    }

    // Tooltip
    if (ImGui::IsMouseHoveringRect(p_min, p_max)) {
        ImGui::BeginTooltip();
        ImGui::Text("ID: %lld", node->land->getId());
        ImGui::Text("Owner: %s", node->land->getRawOwner().c_str());
        ImGui::Text("Pos: %s", node->land->getAABB().toString().c_str());
        ImGui::Text("Type: %d", (int)node->land->getType());
        if (node->land->hasParentLand()) {
            ImGui::Text("Parent: %lld", node->land->getParentLandID());
        }
        ImGui::EndTooltip();
    }
}

} // namespace devtool::viewer