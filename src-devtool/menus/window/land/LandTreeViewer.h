#pragma once
#include "core/Window.h"
#include "pland/Global.h"
#include "pland/land/Land.h"

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

namespace land {
class LandRegistry;
namespace service {
class LandHierarchyService;
}
} // namespace land

namespace devtool::viewer {

class LandTreeViewer : public IWindow {
public:
    LandTreeViewer(std::string title, land::LandID rootId);
    ~LandTreeViewer() = default;

    land::LandID getRootId() const { return mRootId; }

    void render() override;

private:
    struct NodeLayout {
        float                                    x, y;     // 相对画布的逻辑坐标
        float                                    width;    // 子树总宽度
        std::shared_ptr<land::Land>              land;     // 领地数据
        std::vector<std::unique_ptr<NodeLayout>> children; // 子节点
    };

    // 布局计算
    std::unique_ptr<NodeLayout>
          _buildTree(std::shared_ptr<land::Land> const& root, land::service::LandHierarchyService& service);
    float _calculateSubtreeWidth(NodeLayout* node);
    void  _assignCoordinates(NodeLayout* node, float x_offset, float y_depth);

    // 渲染辅助
    void _drawLink(ImDrawList* dl, NodeLayout* parent, NodeLayout* child, const ImVec2& origin, float scale);
    void _drawNode(ImDrawList* dl, NodeLayout* node, const ImVec2& origin, float scale);

private:
    land::LandID mRootId{land::INVALID_LAND_ID};

    // 交互状态
    ImVec2 mScrolling = {0.0f, 0.0f};
    float  mZoom      = 1.0f;

    // 布局缓存 (每帧重建其实很快，但也支持缓存)
    std::unique_ptr<NodeLayout> mLayoutRoot{nullptr};
};

} // namespace devtool::viewer