#include "PermTableEditor.h"
#include "utils/BackUtils.h"

#include "pland/land/repo/LandContext.h"

#include "ll/api/form/CustomForm.h"
#include "ll/api/reflection/Deserialization.h"
#include "ll/api/reflection/Serialization.h"

#include <nlohmann/json.hpp>

namespace land {
namespace gui {

struct PermTableEditor::Impl : std::enable_shared_from_this<Impl> {
    enum class EditType : uint8_t { Environment, Member, Actor };
    Callback      mCallback;
    LandPermTable mTable;

    explicit Impl(Callback callback, LandPermTable const& table)
    : mCallback(std::move(callback)),
      mTable(std::move(table)) {}

    void sendEdit(Player& player, EditType type) {
        auto  localeCode = player.getLocaleCode();
        auto& i18n       = ll::i18n::getInstance();

        auto f = ll::form::CustomForm();
        switch (type) {
        case EditType::Environment:
            f.setTitle("Perm - 全局权限管理"_trl(localeCode));
            break;
        case EditType::Member:
            f.setTitle("Perm - 成员权限管理"_trl(localeCode));
            break;
        case EditType::Actor:
            f.setTitle("Perm - 实体权限管理"_trl(localeCode));
            break;
        }

        auto fields = nlohmann::json::object();
        if (type == EditType::Environment) {
            fields = ll::reflection::serialize<nlohmann::json>(mTable.environment).value();
        } else {
            fields = ll::reflection::serialize<nlohmann::json>(mTable.role).value();
        }

        for (auto& [k, v] : fields.items()) {
            bool value = false;
            switch (type) {
            case EditType::Environment:
                value = v.get<bool>();
                break;
            case EditType::Member:
                static_assert(
                    std::same_as<std::remove_const_t<decltype(std::declval<RolePerms::Entry>().member)>, bool>
                );
                value = v["member"].get<bool>();
                break;
            case EditType::Actor:
                static_assert(
                    std::same_as<std::remove_const_t<decltype(std::declval<RolePerms::Entry>().actor)>, bool>
                );
                value = v["actor"].get<bool>();
                break;
            }
            f.appendToggle(k, std::string{i18n.get(k, localeCode)}, value);
        }

        f.sendTo(
            player,
            [type, thiz = shared_from_this(), fields = std::move(fields)](
                Player&                           player,
                ll::form::CustomFormResult const& data,
                ll::form::FormCancelReason
            ) mutable {
                if (!data) {
                    return;
                }

                // 更新上一步的数据
                for (auto& [k, v] : fields.items()) {
                    bool newVal = std::get<uint64_t>(data->at(k));
                    switch (type) {
                    case EditType::Environment:
                        v = newVal;
                        break;
                    case EditType::Member:
                        v["member"] = newVal;
                        break;
                    case EditType::Actor:
                        v["actor"] = newVal;
                        break;
                    }
                }

                // 反序列化回结构体
                if (type == EditType::Environment) {
                    ll::reflection::deserialize(thiz->mTable.environment, fields).value();
                } else {
                    ll::reflection::deserialize(thiz->mTable.role, fields).value();
                }

                thiz->mCallback(player, thiz->mTable);
            }
        );
    }
};

void PermTableEditor::sendTo(
    Player&                              player,
    LandPermTable const&                 table,
    Callback                             callback,
    ll::form::SimpleForm::ButtonCallback backTo
) {
    auto localeCode = player.getLocaleCode();

    auto impl = std::make_shared<Impl>(std::move(callback), table);

    auto f = ll::form::SimpleForm{};
    f.setTitle("[PLand] - 权限管理"_trl(localeCode));
    f.appendButton(
        "环境权限\n§7世界自动行为与规则"_trl(localeCode),
        "textures/ui/icon_recipe_nature",
        "path",
        [impl](Player& player) { impl->sendEdit(player, Impl::EditType::Environment); }
    );
    f.appendButton(
        "成员权限\n§7拥有领地身份的成员行为"_trl(localeCode),
        "textures/ui/permissions_member_star",
        "path",
        [impl](Player& player) { impl->sendEdit(player, Impl::EditType::Member); }
    );
    f.appendButton(
        "实体权限\n§7影响所有实体(陌生玩家/特殊实体等)"_trl(localeCode),
        "textures/ui/permissions_visitor_hand",
        "path",
        [impl](Player& player) { impl->sendEdit(player, Impl::EditType::Actor); }
    );
    if (backTo) {
        back_utils::injectBackButton(f, std::move(backTo));
    }
    f.sendTo(player);
}

} // namespace gui
} // namespace land