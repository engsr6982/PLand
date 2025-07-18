#include "LandCacheViewer.h"
#include "ll/api/service/PlayerInfo.h"
#include "mc/platform/UUID.h"
#include "pland/PLand.h"
#include "pland/land/LandRegistry.h"


#include <imgui_internal.h>
#include <memory>
#include <string>

namespace devtool::internals {

// LandCacheViewer
LandCacheViewer::LandCacheViewer() : IMenuElement("领地缓存") { window_ = std::make_unique<LandCacheViewerWindow>(); }

bool* LandCacheViewer::getSelectFlag() { return window_->getOpenFlag(); }

bool LandCacheViewer::isSelected() const { return window_->isOpen(); }

void LandCacheViewer::tick() { window_->tick(); }


// LandCacheViewerWindow
LandCacheViewerWindow::LandCacheViewerWindow() { this->setOpenFlag(true); }

void LandCacheViewerWindow::handleButtonClicked(Buttons bt, land::SharedLand land) {
    switch (bt) {
    case EditLand:
        handleEditLand(land);
        break;
    case ExportLand:
        handleExportLand(land);
        break;
    }
}
void LandCacheViewerWindow::handleEditLand(land::SharedLand land) {
    auto id = land->getId();
    if (!editors_.contains(id)) {
        editors_.emplace(id, std::make_unique<LandEditor>(land));
    }
    auto const& editor = editors_[id];
    editor->setOpenFlag(!editor->isOpen());
}
void LandCacheViewerWindow::handleExportLand(land::SharedLand land) {
    namespace fs = std::filesystem;
    auto dir     = land::PLand::getInstance().getSelf().getModDir() / "devtool_exports";
    if (!fs::exists(dir)) {
        fs::create_directory(dir);
    }
    auto          file = dir / fmt::format("land_{}.json", land->getId());
    std::ofstream ofs(file);
    ofs << land->dump().dump(2);
    ofs.close();
}

void LandCacheViewerWindow::preBuildData() {
    lands_ = land::PLand::getInstance().getLandRegistry()->getLandsByOwner();

    auto& playerInfo = ll::service::PlayerInfo::getInstance();
    for (const auto& owner : lands_ | std::views::keys) {
        // 更新玩家名缓存
        if (!realNames_.contains(owner)) {
            auto info         = playerInfo.fromUuid(mce::UUID::fromString(owner));
            realNames_[owner] = info.has_value() ? info->name : owner;
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
    // 移除不存在的窗口
    for (auto iter = editors_.begin(); iter != editors_.end();) {
        if (!iter->second->land_.lock()) {
            iter = editors_.erase(iter); // weak ptr 解锁失败，窗口已移除
        }
        ++iter;
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
            ImGui::SameLine();
            if (ImGui::Button(fmt::format("复制##{}", ld->getId()).c_str())) {
                ImGui::SetClipboardText(ld->dump().dump().c_str());
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
    if (!ImGui::Begin("领地缓存", getOpenFlag())) {
        ImGui::End();
        return;
    }
    preBuildData();
    renderToolBar();
    ImGui::Dummy(ImVec2(0, 5)); // 5像素上间距
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 5)); // 5像素下间距
    renderCacheLand();
    ImGui::End();
}

void LandCacheViewerWindow::tick() {
    IWindow::tick();
    for (auto const& val : editors_ | std::views::values) {
        val->tick();
    }
}


// LandEditor
LandEditor::LandEditor(land::SharedLand land) : CodeEditor(land->dump().dump(4)), land_(land) {}

void LandEditor::renderMenuElement() {
    CodeEditor::renderMenuElement();
    if (ImGui::BeginMenu("Land")) {
        if (ImGui::Button("写入")) {
            auto land = land_.lock();
            if (!land) {
                return;
            }
            auto backup = land->dump();
            try {
                auto json = nlohmann::json::parse(editor_.GetText());
                land->load(json);
            } catch (...) {
                land->load(backup);
                land::PLand::getInstance().getSelf().getLogger().error("Failed to parse json");
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetItemTooltip("将当前更改应用到领地");
        }

        if (ImGui::Button("刷新")) {
            auto land = land_.lock();
            if (!land) {
                return;
            }
            auto json = land->dump();
            this->editor_.SetText(json.dump(4));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetItemTooltip("将当前领地数据刷新到编辑器中");
        }
        ImGui::EndMenu();
    }
}


} // namespace devtool::internals