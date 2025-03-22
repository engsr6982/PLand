#include "pland/gui/CommonUtilGui.hpp"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "mc/world/actor/player/Player.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include <string>

using namespace ll::form;

namespace land {


void EditStringUtilGui::impl(
    Player&                 player,
    string const&           title,        // 标题
    string const&           text,         // 提示
    string const&           defaultValue, // 默认值
    EditStringResult const& callback      // 回调
) {
    CustomForm fm(PLUGIN_NAME + title);
    fm.appendInput("str", text, "string", defaultValue);
    fm.sendTo(player, [callback](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }
        callback(pl, std::get<string>(res->at("str")));
    });
}


void FuzzySerarchUtilGui::impl(Player& player, std::vector<LandData_sptr> const& list, const CallBack& callback) {
    CustomForm fm;
    fm.setTitle(PLUGIN_NAME + " | 模糊搜索领地"_trf(player));
    fm.appendInput("name", "请输入领地名称"_trf(player), "string");
    fm.sendTo(player, [list, callback](Player& player, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        auto                       name = std::get<string>(res->at("name"));
        std::vector<LandData_sptr> filtered;
        for (auto const& ptr : list) {
            if (ptr->getLandName().find(name) != std::string::npos) {
                filtered.push_back(ptr);
            }
        }
        callback(player, filtered);
    });
}


} // namespace land