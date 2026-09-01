#include "LandEditor.h"

#include "pland/PLand.h"
#include "pland/land/Land.h"

#include <fmt/core.h>
#include <utility>

namespace devtool::viewer {

LandEditor::LandEditor(std::shared_ptr<land::Land> land) : CodeEditor(land->toJson().dump(4)), land_(std::move(land)) {
    // 标题在对象生命周期内稳定, 作为停靠/ini 恢复的键
    this->title_ = fmt::format("LandEditor {}", this->windowId_);
}

void LandEditor::renderMenuElement() {
    CodeEditor::renderMenuElement();
    if (ImGui::BeginMenu("Land")) {
        if (ImGui::Button("写入")) {
            auto land = land_.lock();
            if (!land) {
                return;
            }
            auto backup = land->toJson();
            try {
                auto json = nlohmann::json::parse(editor_.GetText());
                land->load(json);
                land->markDirty();
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
            auto json = land->toJson();
            this->editor_.SetText(json.dump(4));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetItemTooltip("将当前领地数据刷新到编辑器中");
        }
        ImGui::EndMenu();
    }
}


} // namespace devtool::viewer