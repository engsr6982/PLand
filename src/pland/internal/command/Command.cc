#include "magic_enum.hpp"

#include "Command.h"
#include "pland/PLand.h"
#include "pland/drawer/DrawHandleManager.h"
#include "pland/events/domain/ConfigReloadEvent.h"
#include "pland/gui/LandBuyGUI.h"
#include "pland/gui/LandMainMenuGUI.h"
#include "pland/gui/LandManagerGUI.h"
#include "pland/gui/NewLandGUI.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/selector/ABSelector.h"
#include "pland/selector/SelectorManager.h"
#include "pland/service/LandManagementService.h"
#include "pland/service/ServiceLocator.h"
#include "pland/utils/FeedbackUtils.h"

#include "ll/api/command/Command.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/io/Logger.h"
#include "ll/api/service/PlayerInfo.h"

#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/utility/optional_ref.h"
#include "mc/platform/UUID.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/agent/agent_commands/Command.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "pland/gui/admin/OperatorManager.h"
#include "pland/service/LeasingService.h"
#include "pland/utils/TimeUtils.h"

#include <memory>
#include <sstream>
#include <vector>


namespace land::internal {

#define CHECK_TYPE(ori, out, type)                                                                                     \
    if (ori.getOriginType() != type) {                                                                                 \
        feedback_utils::sendErrorText(out, "此命令仅限 {} 使用!"_tr(magic_enum::enum_name(type)));                     \
        return;                                                                                                        \
    }

#define GET_AS_PLAYER(ORI) (*static_cast<Player*>(ori.getEntity()))

struct Selector3DLand {
    bool is3DLand{false};
};

namespace handlers {

void open_menu(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = GET_AS_PLAYER(ori);
    gui::LandMainMenuGUI::sendTo(player);
}

void open_admin_mgr(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = GET_AS_PLAYER(ori);
    gui::OperatorManager::sendMainMenu(player);
}

void list_admins(CommandOrigin const& origin, CommandOutput& output) {
    CHECK_TYPE(origin, output, CommandOriginType::DedicatedServer);
    auto& operators = PLand::getInstance().getLandRegistry().getOperators();
    if (operators.empty()) {
        feedback_utils::sendErrorText(output, "当前没有管理员"_tr());
        return;
    }

    std::ostringstream oss;
    oss << "管理员: "_tr();
    auto& infoDb = ll::service::PlayerInfo::getInstance();
    for (auto& oper : operators) {
        auto info = infoDb.fromUuid(oper);
        if (info) {
            oss << info->name;
        } else {
            oss << oper.asString();
        }
        oss << " | ";
    }
    feedback_utils::sendText(output, oss.str());
}

struct AdminActionParam {
    enum class Action { add, remove } action;
    CommandSelector<Player> targets;
};
void admin_add_remove(CommandOrigin const& ori, CommandOutput& out, AdminActionParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    auto& db  = PLand::getInstance().getLandRegistry();
    auto  pls = param.targets.results(ori).data;

    if (pls->empty()) {
        feedback_utils::sendErrorText(out, "未找到任何玩家，请确保玩家在线后重试"_tr());
        return;
    }

    for (auto& pl : *pls) {
        if (!pl) {
            continue;
        }
        auto& uid  = pl->getUuid();
        auto  name = pl->getRealName();
        if (param.action == AdminActionParam::Action::add) {
            if (db.isOperator(uid)) {
                feedback_utils::sendErrorText(out, "{} 已经是管理员，请不要重复添加"_tr(name));
            } else {
                if (db.addOperator(uid)) feedback_utils::sendText(out, "{} 已经被添加为管理员"_tr(name));
            }
        } else {
            if (db.isOperator(uid)) {
                if (db.removeOperator(uid)) feedback_utils::sendText(out, "{} 已经被移除管理员"_tr(name));
            } else {
                feedback_utils::sendErrorText(out, "{} 不是管理员，无需移除"_tr(name));
            }
        }
    }
}


struct NewLandParam {
    enum class NewType : int { Default = 0, SubLand } type{NewType::Default};
};
void new_land(CommandOrigin const& ori, CommandOutput& out, NewLandParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    switch (param.type) {
    case NewLandParam::NewType::Default: {
        gui::NewLandGUI::sendChooseLandDim(player);
        break;
    }

    case NewLandParam::NewType::SubLand: {
        auto expected =
            PLand::getInstance().getServiceLocator().getLandManagementService().requestCreateSubLand(player);
        if (!expected) {
            feedback_utils::sendError(player, expected.error());
            return;
        }
        feedback_utils::sendText(
            player,
            "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trl(
                player.getLocaleCode(),
                ConfigProvider::getSelectionConfig().alias
            )
        );
        break;
    }
    }
}

enum class SetType : int { A, B };
struct SetParam {
    SetType type;
};
void selection_set_point(CommandOrigin const& ori, CommandOutput& out, SetParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    if (!PLand::getInstance().getSelectorManager()->hasSelector(player)) {
        feedback_utils::sendErrorText(
            out,
            "您还没有开启领地选区，请先使用 /pland new 命令"_trl(player.getLocaleCode())
        );
        return;
    }

    auto pos = player.getFeetBlockPos();

    auto selector = PLand::getInstance().getSelectorManager()->getSelector(player);
    assert(selector);

    auto abselector = selector->as<ABSelector>();
    if (!abselector) {
        // TODO: handle thirdparty AbstractSelector
        return;
    }

    if (param.type == SetType::A) {
        abselector->setPointA(pos);
    } else {
        abselector->setPointB(pos);
    }
}

void selection_cancel(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    PLand::getInstance().getSelectorManager()->stopSelection(player);
    feedback_utils::sendText(out, "已取消新建领地"_trl(player.getLocaleCode()));
}


void land_buy(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    gui::LandBuyGUI::sendTo(player);
}

void reload(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    if (PLand::getInstance().loadConfig()) {
        ll::event::EventBus::getInstance().publish(events::ConfigReloadEvent{});
        feedback_utils::sendText(out, "领地系统配置已重新加载"_tr());
    } else {
        feedback_utils::sendErrorText(out, "领地系统配置加载失败，请检查配置文件"_tr());
    }
}


enum class DrawType : int { Disable = 0, NearLand, CurrentLand };
struct DrawParam {
    DrawType type;
};
void land_draw(CommandOrigin const& ori, CommandOutput& out, DrawParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);

    auto& player     = *static_cast<Player*>(ori.getEntity());
    auto  localeCode = player.getLocaleCode();
    auto& db         = PLand::getInstance().getLandRegistry();
    auto  handle     = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(player);

    switch (param.type) {
    case DrawType::Disable: {
        handle->clearLand();
        feedback_utils::sendText(out, "领地绘制已关闭"_trl(localeCode));
        break;
    }

    case DrawType::CurrentLand: {
        auto land = db.getLandAt(player.getPosition(), player.getDimensionId());
        if (!land) {
            feedback_utils::sendErrorText(out, "您当前不在领地内"_trl(localeCode));
            return;
        }
        handle->draw(
            land,
            mce::Color::fromHexString(ConfigProvider::getDrawConfig().color.onUseCommandDrawCurrentLand)
        );
        feedback_utils::sendText(out, "已绘制领地"_trl(localeCode));
        break;
    }

    case DrawType::NearLand: {
        auto lands = db.getLandAt(player.getPosition(), ConfigProvider::getDrawConfig().range, player.getDimensionId());
        for (auto& land : lands)
            handle->draw(
                land,
                mce::Color::fromHexString(ConfigProvider::getDrawConfig().color.onUseCommandDrawNearLand)
            );
        feedback_utils::sendText(out, "已绘制附近 {} 个领地"_trl(localeCode, lands.size()));
        break;
    }
    }
}

void land_set_teleport_pos(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player     = *static_cast<Player*>(ori.getEntity());
    auto  localeCode = player.getLocaleCode();

    auto& mod      = PLand::getInstance();
    auto& registry = mod.getLandRegistry();
    auto  point    = player.getPosition();
    auto  land     = registry.getLandAt(point, player.getDimensionId());
    if (!land) {
        feedback_utils::sendErrorText(out, "您当前不在领地内"_trl(localeCode));
        return;
    }

    auto& service = mod.getServiceLocator().getLandManagementService();
    if (auto res = service.setLandTeleportPos(player, land, point)) {
        feedback_utils::notifySuccess(player, "传送点已更新为: {}"_trl(localeCode, point.toString()));
    } else {
        feedback_utils::sendError(player, res.error());
    }
}

void show_current_land_mgr(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player     = *static_cast<Player*>(ori.getEntity());
    auto  localeCode = player.getLocaleCode();

    auto land = PLand::getInstance().getLandRegistry().getLandAt(player.getPosition(), player.getDimensionId());
    if (!land) {
        feedback_utils::sendText(player, "当前位置没有领地"_trl(localeCode));
        return;
    }

    auto& uuid = player.getUuid();
    if (!land->isOwner(uuid) && !PLand::getInstance().getLandRegistry().isOperator(uuid)) {
        feedback_utils::sendText(player, "当前位置不是你的领地"_trl(localeCode));
        return;
    }

    gui::LandManagerGUI::sendMainMenu(player, land);
}

using ll::command::RuntimeCommand;

bool ensure_origin_console_or_player(CommandOrigin const& ori, CommandOutput& out) {
    auto originType = ori.getOriginType();
    if (originType != CommandOriginType::DedicatedServer && originType != CommandOriginType::Player) {
        feedback_utils::sendErrorText(out, "This command can only be executed by players and the console"_tr());
        return false;
    }
    return true;
}

bool ensure_admin(CommandOrigin const& ori, CommandOutput& out) {
    if (ori.getOriginType() == CommandOriginType::Player) {
        auto& player = GET_AS_PLAYER(ori);
        if (!PLand::getInstance().getLandRegistry().isOperator(player.getUuid())) {
            feedback_utils::sendErrorText(out, "You do not have permission to execute this command"_tr());
            return false;
        }
    }
    return true;
}

std::shared_ptr<Land> resolve_lease_land(
    CommandOrigin const&  ori,
    CommandOutput&        out,
    RuntimeCommand const& param,
    std::string const&    idKey = "id"
) {
    if (!ensure_origin_console_or_player(ori, out)) return nullptr;

    auto& registry = PLand::getInstance().getLandRegistry();

    if (auto& optID = param[idKey]) {
        // 指定 ID，进行精确查询
        LandID id = std::get<int>(optID.value());

        if (auto land = registry.getLand(id)) {
            // 非控制台执行此命令，校验权限
            if (ori.getOriginType() == CommandOriginType::Player) {
                auto& player = GET_AS_PLAYER(ori);
                if (!land->isOwner(player.getUuid()) && !registry.isOperator(player.getUuid())) {
                    feedback_utils::sendErrorText(out, "您没有权限操作此领地"_tr());
                    return nullptr;
                }
            }
            return land;
        } else {
            feedback_utils::sendErrorText(out, "指定的领地 ID 不存在"_tr());
            return nullptr;
        }
    } else if (ori.getOriginType() == CommandOriginType::Player) {
        // 未指定 ID，进行当前位置查询
        auto& player = GET_AS_PLAYER(ori);
        if (auto land = registry.getLandAt(player.getPosition(), player.getDimensionId())) {
            return land;
        } else {
            feedback_utils::sendErrorText(out, "当前位置没有领地"_tr());
            return nullptr;
        }
    } else {
        feedback_utils::sendErrorText(out, "仅玩家可以查询当前位置的领地"_tr());
        return nullptr;
    }
}

void show_lease_info(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;

    auto land = resolve_lease_land(ori, out, param);
    if (!land) return;

    if (!land->isLeased()) {
        feedback_utils::sendErrorText(out, "当前领地不是租赁模式"_tr());
        return;
    }

    auto&       infoDb = ll::service::PlayerInfo::getInstance();
    std::string displayName;
    if (land->isSystemOwned()) {
        displayName = "系统"_tr();
    } else if (auto info = infoDb.fromUuid(land->getOwner())) {
        displayName = info->name;
    } else {
        displayName = land->getOwner().asString();
    }

    out.success("---- 租赁信息 ----"_tr());
    out.success("领地ID: {}"_tr(land->getId()));
    out.success("领地名称: {}"_tr(land->getName()));
    out.success("所有者: {}"_tr(displayName));
    out.success("租赁状态: {}"_tr(magic_enum::enum_name(land->getLeaseState())));
    out.success("租赁起始时间: {}"_tr(time_utils::formatTime(time_utils::toClockTime(land->getLeaseStartAt()))));
    out.success("租赁结束时间: {}"_tr(time_utils::formatTime(time_utils::toClockTime(land->getLeaseEndAt()))));
    out.success("剩余租期: {}"_tr(time_utils::formatRemaining(land->getLeaseEndAt())));
}

void admin_set_lease_start_end(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;
    if (!ensure_admin(ori, out)) return;

    auto land = resolve_lease_land(ori, out, param);
    if (!land) return;

    auto& target  = std::get<ll::command::RuntimeEnum>(param["set_target"].value());
    bool  isStart = target.name == "set_start";

    auto date  = std::get<std::string>(param["date"].value());
    auto clock = time_utils::parseTime(date);
    if (clock == std::chrono::system_clock::time_point{}) {
        feedback_utils::sendErrorText(out, "无效的日期格式"_tr());
        return;
    }

    auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
    if (isStart) {
        if (auto exp = service.setStartAt(land, clock)) {
            feedback_utils::sendText(out, "领地租赁起始时间已修改为: {}"_tr(time_utils::formatTime(clock)));
        } else {
            feedback_utils::sendError(out, exp.error());
        }
    } else {
        if (auto exp = service.setEndAt(land, clock)) {
            feedback_utils::sendText(out, "领地租赁结束时间已修改为: {}"_tr(time_utils::formatTime(clock)));
        } else {
            feedback_utils::sendError(out, exp.error());
        }
    }
}

enum class AdminLeaseAddTimeUint { day, hour, min, sec };
void admin_add_lease_time(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;
    if (!ensure_admin(ori, out)) return;

    auto land = resolve_lease_land(ori, out, param);
    if (!land) return;

    auto amount = std::get<int>(param["amount"].value());
    if (amount <= 0) {
        feedback_utils::sendErrorText(out, "无效的时间数量"_tr());
        return;
    }

    long long sec = 0;

    auto rawUint = std::get<ll::command::RuntimeEnum>(param["uint"].value());
    auto uint    = static_cast<AdminLeaseAddTimeUint>(rawUint.index);
    switch (uint) {
    case AdminLeaseAddTimeUint::day:
        sec = time_utils::toSeconds(amount);
        break;
    case AdminLeaseAddTimeUint::hour:
        sec = amount * time_utils::MinutesPerHour * time_utils::SecondsPerMinute;
        break;
    case AdminLeaseAddTimeUint::min:
        sec = amount * time_utils::SecondsPerMinute;
        break;
    case AdminLeaseAddTimeUint::sec:
        sec = amount;
        break;
    default:
        feedback_utils::sendErrorText(out, "无效的时间单位"_tr());
        return;
    }

    auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
    if (auto exp = service.addTime(land, sec)) {
        feedback_utils::sendText(out, "领地租赁时间已延长 {} 秒"_tr(sec));
        feedback_utils::sendText(
            out,
            "领地到期时间: {}"_tr(time_utils::formatTime(time_utils::toClockTime(land->getLeaseEndAt())))
        );
    } else {
        feedback_utils::sendError(out, exp.error());
    }
}

enum class AdminLeaseForceTarget {
    force_freeze,
    force_recycle,
};
void admin_force_freeze_or_recycle(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;
    if (!ensure_admin(ori, out)) return;

    auto land = resolve_lease_land(ori, out, param);
    if (!land) return;

    auto force_target = std::get<ll::command::RuntimeEnum>(param["force_target"].value());
    auto target       = static_cast<AdminLeaseForceTarget>(force_target.index);

    auto& service = PLand::getInstance().getServiceLocator().getLeasingService();

    switch (target) {
    case AdminLeaseForceTarget::force_freeze:
        if (auto exp = service.forceFreeze(land)) {
            feedback_utils::sendText(out, "领地 '{}(ID: {})' 已强制冻结"_tr(land->getName(), land->getId()));
        } else {
            feedback_utils::sendError(out, exp.error());
        }
        break;
    case AdminLeaseForceTarget::force_recycle:
        if (auto exp = service.forceRecycle(land)) {
            feedback_utils::sendText(out, "领地 '{}(ID: {})' 已强制回收"_tr(land->getName(), land->getId()));
        } else {
            feedback_utils::sendError(out, exp.error());
        }
        break;
    default:
        feedback_utils::sendErrorText(out, "无效的强制目标"_tr());
    }
}

void admin_clean_lease(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;
    if (!ensure_admin(ori, out)) return;

    auto days = std::get<int>(param["days"].value());

    auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
    if (auto exp = service.cleanExpiredLands(days)) {
        feedback_utils::sendText(out, "已删除 {} 个过期领地"_tr(exp.value()));
    } else {
        feedback_utils::sendError(out, exp.error());
    }
}

void admin_to_bought(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;
    if (!ensure_admin(ori, out)) return;

    auto land = resolve_lease_land(ori, out, param);
    if (!land) return;

    auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
    if (auto exp = service.toBought(land)) {
        feedback_utils::sendText(out, "领地 '{}(ID: {})' 已转为购买领地"_tr(land->getName(), land->getId()));
    } else {
        feedback_utils::sendError(out, exp.error());
    }
}

void admin_to_leased(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;
    if (!ensure_admin(ori, out)) return;

    auto land = resolve_lease_land(ori, out, param);
    if (!land) return;

    auto days = std::get<int>(param["days"].value());

    auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
    if (auto exp = service.toLeased(land, days)) {
        feedback_utils::sendText(
            out,
            "领地 '{}(ID: {})' 已转为租赁领地，租期 {} 天"_tr(land->getName(), land->getId(), days)
        );
    } else {
        feedback_utils::sendError(out, exp.error());
    }
}

void admin_snapshot_db(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!ensure_origin_console_or_player(ori, out)) return;
    if (!ensure_admin(ori, out)) return;

    std::optional<std::string> dirName;
    if (auto& dir = param["dirName"]) {
        dirName = std::get<std::string>(dir.value());
    }

    PLand::getInstance().getLandRegistry().createSnapshot(dirName);
    feedback_utils::sendText(out, "The task has been created, please wait for it to complete..."_tr());
}

}; // namespace handlers


bool LandCommand::setup() {
    auto& registrar = ll::command::CommandRegistrar::getInstance(false);
    auto& h         = registrar.getOrCreateCommand("pland", "PLand - 领地系统"_tr());
    h.alias("land");

    // pland reload
    h.overload().text("reload").execute(&handlers::reload);

    // pland [gui|menu] 领地GUI
    h.overload().execute(&handlers::open_menu);
    h.overload().text("gui").execute(&handlers::open_menu);
    h.overload().text("menu").execute(&handlers::open_menu);

    // pland mgr 领地管理GUI
    h.overload().text("mgr").execute(&handlers::open_admin_mgr);

    // pland admin list 列出所有管理员
    h.overload().text("admin").text("list").execute(&handlers::list_admins);

    // pland admin <add|remove> <targets> 添加/移除管理员
    h.overload<handlers::AdminActionParam>().text("admin").required("action").required("targets").execute(
        &handlers::admin_add_remove
    );

    // pland new [NewType: type] 新建一个领地
    h.overload<handlers::NewLandParam>().text("new").optional("type").execute(&handlers::new_land);

    // pland this 获取当前领地信息
    h.overload().text("this").execute(&handlers::show_current_land_mgr);

    // pland set <a/b> 选点a/b
    h.overload<handlers::SetParam>().text("set").required("type").execute(&handlers::selection_set_point);

    // pland cancel 取消新建
    h.overload().text("cancel").execute(&handlers::selection_cancel);

    // pland buy 购买
    h.overload().text("buy").execute(&handlers::land_buy);

    // pland draw <disable|near|current> 开启/关闭领地绘制
    if (ConfigProvider::getDrawConfig().enabled) {
        h.overload<handlers::DrawParam>().text("draw").required("type").execute(&handlers::land_draw);
    }

    // pland set teleport_pos 设置传送点
    h.overload().text("set").text("teleport_pos").execute(&handlers::land_set_teleport_pos);

    if (ConfigProvider::getLeasingConfig().enabled) {
        // pland lease info [id] 查看当前/指定领地的租赁信息
        h.runtimeOverload()
            .text("lease")
            .text("info")
            .optional("id", ll::command::ParamKind::Int)
            .execute(&handlers::show_lease_info);

        // pland admin lease <set_start|set_end> <timestamp|YYYY-MM-DD HH:mm:ss> [id] 设置领地租赁 开启/结束 时间
        if (!registrar.hasEnum("pland_lease_set_target")) {
            registrar.tryRegisterRuntimeEnum(
                "pland_lease_set_target",
                {
                    {"set_start", 0},
                    {  "set_end", 1}
            }
            );
        }
        h.runtimeOverload()
            .text("admin")
            .text("lease")
            .required("set_target", ll::command::ParamKind::Enum, "pland_lease_set_target")
            .required("date", ll::command::ParamKind::String)
            .optional("id", ll::command::ParamKind::Int)
            .execute(&handlers::admin_set_lease_start_end);

        // pland admin lease add_time <amount> <day|hour|min|sec> [id] 增加领地租赁时间
        registrar.tryRegisterRuntimeEnum<handlers::AdminLeaseAddTimeUint>();
        h.runtimeOverload()
            .text("admin")
            .text("lease")
            .text("add_time")
            .required("amount", ll::command::ParamKind::Int)
            .required("uint", ll::command::ParamKind::Enum, ll::command::enum_name_v<handlers::AdminLeaseAddTimeUint>)
            .optional("id", ll::command::ParamKind::Int)
            .execute(&handlers::admin_add_lease_time);

        // pland admin lease <force_freeze|force_recycle> [id] 强制冻结/回收领地
        registrar.tryRegisterRuntimeEnum<handlers::AdminLeaseForceTarget>();
        h.runtimeOverload()
            .text("admin")
            .text("lease")
            .required(
                "force_target",
                ll::command::ParamKind::Enum,
                ll::command::enum_name_v<handlers::AdminLeaseForceTarget>
            )
            .optional("id", ll::command::ParamKind::Int)
            .execute(&handlers::admin_force_freeze_or_recycle);

        // pland admin lease clean <days> 回收到期超过n天的领地
        h.runtimeOverload()
            .text("admin")
            .text("lease")
            .text("clean")
            .required("days", ll::command::ParamKind::Int)
            .execute(&handlers::admin_clean_lease);

        // pland admin lease to_bought [id] 将租赁领地转为购买领地
        h.runtimeOverload()
            .text("admin")
            .text("lease")
            .text("to_bought")
            .optional("id", ll::command::ParamKind::Int)
            .execute(&handlers::admin_to_bought);

        // pland admin lease to_leased <days> [id] 将购买领地转为租赁领地
        h.runtimeOverload()
            .text("admin")
            .text("lease")
            .text("to_leased")
            .required("days", ll::command::ParamKind::Int)
            .optional("id", ll::command::ParamKind::Int)
            .execute(&handlers::admin_to_leased);
    }

    // pland admin snapshot_db [dirName: string] 备份领地数据库
    h.runtimeOverload()
        .text("admin")
        .text("snapshot_db")
        .optional("dirName", ll::command::ParamKind::String)
        .execute(&handlers::admin_snapshot_db);

#ifdef LD_DEVTOOL
    // pland devtool
    if (ConfigProvider::isDevToolsEnabled()) {
        h.overload().text("devtool").execute([](CommandOrigin const& ori, CommandOutput&) {
            if (ori.getOriginType() == CommandOriginType::DedicatedServer) {
                auto& mod = PLand::getInstance();
                mod.setDevToolVisible(true);
            }
        });
    }
#endif

    return true;
}


} // namespace land::internal
