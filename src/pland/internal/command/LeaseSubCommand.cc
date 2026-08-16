#include "Command.h"

#include "pland/PLand.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/service/LeasingService.h"
#include "pland/service/ServiceLocator.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/TimeUtils.h"


#include "ll/api/command/Command.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "ll/api/service/PlayerInfo.h"

#include "mc/platform/UUID.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandSelector.h"

namespace land::internal {

namespace {

using ll::command::RuntimeCommand;

std::shared_ptr<Land> resolve_lease_land(
    CommandOrigin const&  ori,
    CommandOutput&        out,
    RuntimeCommand const& param,
    std::string const&    idKey = "id"
) {
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return nullptr;

    auto& registry = PLand::getInstance().getLandRegistry();

    if (auto& optID = param[idKey]) {
        // 指定 ID，进行精确查询
        LandID id = std::get<int>(optID.value());

        if (auto land = registry.getLand(id)) {
            // 非控制台执行此命令，校验权限
            if (ori.getOriginType() == CommandOriginType::Player) {
                auto& player = *static_cast<Player*>(ori.getEntity());
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
        auto& player = *static_cast<Player*>(ori.getEntity());
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
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return;

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
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return;
    if (!LandCommand::ensure_admin(ori, out)) return;

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
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return;
    if (!LandCommand::ensure_admin(ori, out)) return;

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
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return;
    if (!LandCommand::ensure_admin(ori, out)) return;

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
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return;
    if (!LandCommand::ensure_admin(ori, out)) return;

    auto days = std::get<int>(param["days"].value());

    auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
    if (auto exp = service.cleanExpiredLands(days)) {
        feedback_utils::sendText(out, "已删除 {} 个过期领地"_tr(exp.value()));
    } else {
        feedback_utils::sendError(out, exp.error());
    }
}

void admin_to_bought(CommandOrigin const& ori, CommandOutput& out, RuntimeCommand const& param) {
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return;
    if (!LandCommand::ensure_admin(ori, out)) return;

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
    if (!LandCommand::ensure_origin_console_or_player(ori, out)) return;
    if (!LandCommand::ensure_admin(ori, out)) return;

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

} // namespace

void LandCommand::setupLeaseSubCommands(ll::command::CommandRegistrar& reg, ll::command::CommandHandle& h) {
    if (!ConfigProvider::getLeasingConfig().enabled) {
        return; // 租赁功能未启用
    }

    // pland lease info [id] 查看当前/指定领地的租赁信息
    h.runtimeOverload()
        .text("lease")
        .text("info")
        .optional("id", ll::command::ParamKind::Int)
        .execute(&show_lease_info);

    // pland admin lease <set_start|set_end> <timestamp|YYYY-MM-DD HH:mm:ss> [id] 设置领地租赁 开启/结束 时间
    if (!reg.hasEnum("pland_lease_set_target")) {
        reg.tryRegisterRuntimeEnum(
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
        .execute(&admin_set_lease_start_end);

    // pland admin lease add_time <amount> <day|hour|min|sec> [id] 增加领地租赁时间
    reg.tryRegisterRuntimeEnum<AdminLeaseAddTimeUint>();
    h.runtimeOverload()
        .text("admin")
        .text("lease")
        .text("add_time")
        .required("amount", ll::command::ParamKind::Int)
        .required("uint", ll::command::ParamKind::Enum, ll::command::enum_name_v<AdminLeaseAddTimeUint>)
        .optional("id", ll::command::ParamKind::Int)
        .execute(&admin_add_lease_time);

    // pland admin lease <force_freeze|force_recycle> [id] 强制冻结/回收领地
    reg.tryRegisterRuntimeEnum<AdminLeaseForceTarget>();
    h.runtimeOverload()
        .text("admin")
        .text("lease")
        .required("force_target", ll::command::ParamKind::Enum, ll::command::enum_name_v<AdminLeaseForceTarget>)
        .optional("id", ll::command::ParamKind::Int)
        .execute(&admin_force_freeze_or_recycle);

    // pland admin lease clean <days> 回收到期超过n天的领地
    h.runtimeOverload()
        .text("admin")
        .text("lease")
        .text("clean")
        .required("days", ll::command::ParamKind::Int)
        .execute(&admin_clean_lease);

    // pland admin lease to_bought [id] 将租赁领地转为购买领地
    h.runtimeOverload()
        .text("admin")
        .text("lease")
        .text("to_bought")
        .optional("id", ll::command::ParamKind::Int)
        .execute(&admin_to_bought);

    // pland admin lease to_leased <days> [id] 将购买领地转为租赁领地
    h.runtimeOverload()
        .text("admin")
        .text("lease")
        .text("to_leased")
        .required("days", ll::command::ParamKind::Int)
        .optional("id", ll::command::ParamKind::Int)
        .execute(&admin_to_leased);
}


} // namespace land::internal