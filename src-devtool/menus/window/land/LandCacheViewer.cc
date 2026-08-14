#include "LandCacheViewer.h"
#include "LandEditor.h"
#include "LandTreeViewer.h"

#include "core/WindowManager.h"
#include "pland/PLand.h"
#include "pland/land/repo/LandRegistry.h"

#include "mc/platform/UUID.h"

#include "ll/api/service/PlayerInfo.h"

#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <imgui_internal.h>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace devtool::menus {

// 领地缓存查看器; 动态窗口(LandEditor/LandTreeViewer)所有权在 WindowManager, 此处仅登记索引
class LandCacheViewerWindow : public IWindow {
    std::unordered_map<mce::UUID, std::unordered_set<std::shared_ptr<land::Land>>> lands_;     // 领地缓存
    std::unordered_map<mce::UUID, std::string>                                     realNames_; // 玩家名缓存
    std::unordered_map<mce::UUID, bool>                                            isShow_;    // 是否显示该玩家的领地
    std::unordered_map<land::LandID, devtool::viewer::LandEditor*>                 editors_;   // 领地数据编辑器
    std::unordered_map<land::LandID, devtool::viewer::LandTreeViewer*>             viewers_;   // 领地树形视图

    WindowManager& wm_;

    bool showAllPlayerLand_{true}; // 是否显示所有玩家的领地
    bool showOrdinaryLand_{true};  // 是否显示普通领地
    bool showParentLand_{true};    // 是否显示父领地
    bool showMixLand_{true};       // 是否显示混合领地
    bool showSubLand_{true};       // 是否显示子领地
    int  dimensionFilter_{-1};     // 维度过滤
    int  idFilter_{-1};            // 领地ID过滤

public:
    LandCacheViewerWindow(std::string title, WindowManager& wm);
    ~LandCacheViewerWindow() override;

    enum Buttons {
        EditLand,  // 编辑领地数据
        ExportLand // 导出领地数据
    };
    void handleButtonClicked(Buttons bt, std::shared_ptr<land::Land> land);

    void handleEditLand(std::shared_ptr<land::Land> land);
    void handleExportLand(std::shared_ptr<land::Land> land);
    void handleViewLandTree(std::shared_ptr<land::Land> land);

    void renderCacheLand(); // 渲染缓存的领地

    void renderToolBar(); // 渲染工具栏

    void preBuildData(); // 预处理数据

    void render() override;
};

LandCacheViewer::LandCacheViewer(WindowManager& wm) : IMenuElement("领地缓存") {
    auto& wnd = wm.create<LandCacheViewerWindow>("领地缓存", wm);
    this->setWindow(&wnd);
}


// LandCacheViewerWindow
LandCacheViewerWindow::LandCacheViewerWindow(std::string title, WindowManager& wm)
: IWindow(std::move(title)),
  wm_(wm) {
    this->setVisible(true); // 默认打开(持久化配置可覆盖)
}

LandCacheViewerWindow::~LandCacheViewerWindow() = default;

void LandCacheViewerWindow::handleButtonClicked(Buttons bt, std::shared_ptr<land::Land> land) {
    switch (bt) {
    case EditLand:
        handleEditLand(land);
        break;
    case ExportLand:
        handleExportLand(land);
        break;
    }
}
void LandCacheViewerWindow::handleEditLand(std::shared_ptr<land::Land> land) {
    auto id = land->getId();
    if (!editors_.contains(id)) {
        // 所有权移交 WindowManager, 并停靠进本窗口当前节点(父子停靠)
        auto& editor = wm_.registerOwned(std::make_unique<devtool::viewer::LandEditor>(land));
        wm_.requestDockInto(this, &editor);
        editors_.emplace(id, &editor);
    }
    editors_[id]->setVisible(!editors_[id]->visible());
}

void LandCacheViewerWindow::handleViewLandTree(std::shared_ptr<land::Land> land) {
    auto id = land->getId();
    if (!viewers_.contains(id)) {
        auto& viewer = wm_.registerOwned(
            std::make_unique<devtool::viewer::LandTreeViewer>(fmt::format("LandTreeViewer {}", id), id)
        );
        wm_.requestDockInto(this, &viewer);
        viewers_.emplace(id, &viewer);
    }
    viewers_[id]->setVisible(true);
}

void LandCacheViewerWindow::handleExportLand(std::shared_ptr<land::Land> land) {
    namespace fs = std::filesystem;
    auto dir     = land::PLand::getInstance().getSelf().getModDir() / "devtool_exports";
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }
    auto          file = dir / fmt::format("land_{}.json", land->getId());
    std::ofstream ofs(file);
    ofs << land->toJson().dump(2);
    ofs.close();
}

void LandCacheViewerWindow::preBuildData() {
    lands_ = land::PLand::getInstance().getLandRegistry().getLandsByOwner();

    auto& playerInfo = ll::service::PlayerInfo::getInstance();
    for (const auto& owner : lands_ | std::views::keys) {
        // 更新玩家名缓存
        if (!realNames_.contains(owner)) {
            if (owner == land::SYSTEM_ACCOUNT_UUID) {
                realNames_[owner] = "PLandSystem";
            } else {
                auto info         = playerInfo.fromUuid(owner);
                realNames_[owner] = info.has_value() ? info->name : owner.asString();
            }
        }
        // 更新 CheckBox
        if (!isShow_.contains(owner)) {
            isShow_[owner] = false;
        }
    }

    // 移除不存在的玩家
    for (auto iter = realNames_.begin(); iter != realNames_.end();) {
        if (!lands_.contains(iter->first)) {
            iter = realNames_.erase(iter); // 玩家不存在，删除缓存
        }
        ++iter;
    }
    // 同步移除 CheckBox
    for (auto iter = isShow_.begin(); iter != isShow_.end();) {
        if (!lands_.contains(iter->first)) {
            iter = isShow_.erase(iter); // 玩家不存在，删除 CheckBox
        }
        ++iter;
    }
    // 移除不存在的窗口(注销后由 WindowManager 帧末销毁)
    for (auto iter = editors_.begin(); iter != editors_.end();) {
        if (!iter->second->land_.lock()) { // weak ptr 解锁失败，窗口已移除
            wm_.unregister(iter->second);
            iter = editors_.erase(iter);
        } else {
            ++iter;
        }
    }
}

void LandCacheViewerWindow::renderToolBar() {
    ImGui::BeginGroup();
    if (ImGui::Button("重置")) {
        showAllPlayerLand_ = true;
        showOrdinaryLand_  = true;
        showParentLand_    = true;
        showMixLand_       = true;
        showSubLand_       = true;
        dimensionFilter_   = -1;
        idFilter_          = -1;
        isShow_.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("普通领地", &showOrdinaryLand_);
    ImGui::SameLine();
    ImGui::Checkbox("父领地", &showParentLand_);
    ImGui::SameLine();
    ImGui::Checkbox("混合领地", &showMixLand_);
    ImGui::SameLine();
    ImGui::Checkbox("子领地", &showSubLand_);
    ImGui::SameLine(0, 20);
    if (ImGui::BeginCombo("玩家过滤", "impl", ImGuiComboFlags_NoPreview)) {
        ImGui::Checkbox("显示所有玩家", &showAllPlayerLand_);
        int i = 0;
        for (auto& [owner, show] : isShow_) {
            ImGui::PushID(i++);
            ImGui::Checkbox((realNames_[owner]).c_str(), &show);
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine(0, 20);
    ImGui::SetNextItemWidth(130);
    ImGui::InputInt("维度过滤", &dimensionFilter_);
    ImGui::SameLine(0, 20);
    ImGui::SetNextItemWidth(130);
    ImGui::InputInt("领地ID查询", &idFilter_);
    ImGui::EndGroup();
}

void LandCacheViewerWindow::renderCacheLand() {
    static ImGuiTableFlags flags = ImGuiTableFlags_Resizable |   // 列大小调整功能
                                   ImGuiTableFlags_Reorderable | // 表头行中重新排列列
                                   ImGuiTableFlags_Borders |     // 绘制所有边框
                                   ImGuiTableFlags_ScrollY;      // 启用垂直滚动，以便固定表头

    if (!ImGui::BeginTable("land_cache", 7, flags)) {
        return;
    }

    // 设置表头
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("玩家", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("类别", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("维度", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("坐标", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    // 渲染表
    for (const auto& [owner, lands] : lands_) {
        if (!showAllPlayerLand_ && !isShow_[owner]) {
            continue;
        }

        auto const& name = realNames_[owner];
        for (const auto& ld : lands) {
            if ((!showOrdinaryLand_ && ld->isOrdinaryLand()) || (!showParentLand_ && ld->isParentLand())
                || (!showMixLand_ && ld->isMixLand()) || (!showSubLand_ && ld->isSubLand())) {
                continue;
            }
            if ((dimensionFilter_ != -1 && ld->getDimensionId() != dimensionFilter_)
                || (idFilter_ != -1 && ld->getId() != idFilter_)) {
                continue;
            }

            // 渲染表行
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); // ID
            ImGui::Text("%llu", ld->getId());
            ImGui::TableNextColumn(); // 玩家名
            ImGui::Text("%s", name.c_str());
            ImGui::TableNextColumn(); // 名称
            ImGui::Text("%s", ld->getName().c_str());
            ImGui::TableNextColumn(); // 类别
            ImGui::Text(
                "%s",
                ld->isOrdinaryLand() ? "普通领地"
                : ld->isParentLand() ? "父领地"
                : ld->isMixLand()    ? "混合领地"
                : ld->isSubLand()    ? "子领地"
                                     : "未知"
            );
            ImGui::TableNextColumn(); // 维度
            ImGui::Text("%i", ld->getDimensionId());
            ImGui::TableNextColumn(); // 坐标
            ImGui::Text("%s", ld->getAABB().toString().c_str());
            ImGui::TableNextColumn(); // 操作
            if (ImGui::Button(fmt::format("编辑数据##{}", ld->getId()).c_str())) {
                handleButtonClicked(EditLand, ld);
            }
            if (ld->isParentLand()) {
                ImGui::SameLine();
                if (ImGui::Button(fmt::format("查看领地树##{}", ld->getId()).c_str())) {
                    handleViewLandTree(ld);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(fmt::format("复制##{}", ld->getId()).c_str())) {
                ImGui::SetClipboardText(ld->toJson().dump().c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button(fmt::format("导出##{}", ld->getId()).c_str())) {
                handleButtonClicked(ExportLand, ld);
            }
            if (ImGui::IsItemHovered()) ImGui::SetItemTooltip("将当前领地数据导出到 pland/devtool_exports 下");
        }
    }
    ImGui::EndTable();
}

void LandCacheViewerWindow::render() {
    WindowScope scope(*this);
    if (!scope.isOpen()) {
        return;
    }
    preBuildData();
    renderToolBar();
    ImGui::Dummy(ImVec2(0, 5)); // 5像素上间距
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 5)); // 5像素下间距
    renderCacheLand();
}


} // namespace devtool::menus
