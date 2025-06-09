#include "pland/Command.h"
#include "Pland/gui/BuyLandGui.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/service/Bedrock.h"
#include "magic_enum.hpp"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/core/utility/optional_ref.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/packet/SetTimePacket.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandPositionFloat.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/agent/agent_commands/Command.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/ChunkBlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mod/ModEntry.h"
#include "pland/Config.h"
#include "pland/DataConverter.h"
#include "pland/DrawHandleManager.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/gui/LandManageGui.h"
#include "pland/utils/McUtils.h"
#include "pland/utils/Utils.h"
#include <filesystem>
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/i18n/I18n.h>
#include <ll/api/io/Logger.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/service/PlayerInfo.h>
#include <ll/api/service/Service.h>
#include <ll/api/utils/HashUtils.h>
#include <mc/network/packet/LevelChunkPacket.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/server/ServerLevel.h>
#include <mc/server/ServerPlayer.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOriginType.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandParameterOption.h>
#include <mc/server/commands/CommandPermissionLevel.h>
#include <mc/server/commands/CommandRegistry.h>
#include <mc/server/commands/CommandSelector.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/ActorType.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/GameType.h>
#include <memory>
#include <sstream>


#ifdef LD_DEVTOOL
#include "DevToolApp.h"
#include "DevToolAppManager.h"
#endif


namespace land {

#define CHECK_TYPE(ori, out, type)                                                                                     \
    if (ori.getOriginType() != type) {                                                                                 \
        mc_utils::sendText(out, "此命令仅限 {} 使用!"_tr(magic_enum::enum_name(type)));                                \
        return;                                                                                                        \
    }


struct Selector3DLand {
    bool is3DLand{false};
};

namespace Lambda {

static auto const Root = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    LandMainGui::impl(player);
};


static auto const Mgr = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    LandOPManagerGui::impl(player);
};


enum class OperatorType : int { Op, Deop };
struct OperatorParam {
    OperatorType            type;
    CommandSelector<Player> player;
};
static auto const Operator = [](CommandOrigin const& ori, CommandOutput& out, OperatorParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    auto& db  = PLand::getInstance();
    auto  pls = param.player.results(ori).data;

    if (pls->empty()) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(out, "未找到任何玩家，请确保玩家在线后重试"_tr());
        return;
    }

    for (auto& pl : *pls) {
        if (!pl) {
            continue;
        }
        auto uid  = pl->getUuid().asString();
        auto name = pl->getRealName();
        if (param.type == OperatorType::Op) {
            if (db.isOperator(uid)) {
                mc_utils::sendText(out, "{} 已经是管理员，请不要重复添加"_tr(name));
            } else {
                if (db.addOperator(uid)) mc_utils::sendText(out, "{} 已经被添加为管理员"_tr(name));
            }
        } else {
            if (db.isOperator(uid)) {
                if (db.removeOperator(uid)) mc_utils::sendText(out, "{} 已经被移除管理员"_tr(name));
            } else {
                mc_utils::sendText(out, "{} 不是管理员，无需移除"_tr(name));
            }
        }
    }
};

static auto const ListOperator = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    auto pls = PLand::getInstance().getOperators();
    if (pls.empty()) {
        mc_utils::sendText(out, "当前没有管理员"_tr());
        return;
    }

    std::ostringstream oss;
    oss << "管理员: "_tr();
    auto& infoDb = ll::service::PlayerInfo::getInstance();
    for (auto& pl : pls) {
        auto info = infoDb.fromUuid(pl);
        if (info) {
            oss << info->name;
        } else {
            oss << pl;
        }
        oss << " | ";
    }
    mc_utils::sendText(out, oss.str());
    return;
};


enum class NewType : int { Default = 0, SubLand };
struct NewParam {
    NewType type = NewType::Default;
};
static auto const New = [](CommandOrigin const& ori, CommandOutput& out, NewParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    switch (param.type) {
    case NewType::Default: {
        ChooseLandDimAndNewLand::impl(player);
        break;
    }

    case NewType::SubLand: {
        if (!Config::cfg.land.subLand.enabled) {
            mc_utils::sendText(out, "子领地功能未启用，请联系管理员"_trf(player));
            return;
        }

        auto land = PLand::getInstance().getLandAt(player.getPosition(), player.getDimensionId());
        if (!land) {
            mc_utils::sendText(out, "当前位置没有领地"_trf(player));
            return;
        }

        auto uuidStr = player.getUuid().asString();
        if (!land->isLandOwner(uuidStr)) {
            mc_utils::sendText(out, "当前位置不是你的领地"_trf(player));
            return;
        }

        if (!land->canCreateSubLand()) {
            mc_utils::sendText(out, "当前领地无法创建子领地"_trf(player));
            return;
        }

        auto selector = std::make_unique<SubLandSelector>(player, land);
        if (SelectorManager::getInstance().start(std::move(selector))) {
            mc_utils::sendText(
                player,
                "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trf(player, Config::cfg.selector.tool)
            );
        } else {
            mc_utils::sendText(player, "选区开启失败，当前存在未完成的选区任务"_trf(player));
        }

        break;
    }

    default:
        // UnImplemented
        break;
    }
};

enum class SetType : int { A, B };
struct SetParam {
    SetType type;
};
static auto const Set = [](CommandOrigin const& ori, CommandOutput& out, SetParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player   = *static_cast<Player*>(ori.getEntity());
    auto  selector = SelectorManager::getInstance().get(player);
    if (!selector) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(
            out,
            "您还没有开启领地选区，请先使用 /pland new 命令"_trf(player)
        );
        return;
    }
    auto pos = player.getFeetBlockPos();

    if (param.type == SetType::A) {
        selector->selectPointA(pos);
    } else {
        selector->selectPointB(pos);
        selector->onABSelected();
    }

    mc_utils::sendText(out, "已选择点{}"_trf(player, param.type == SetType::A ? "A" : "B"));
};

static auto const Cancel = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    SelectorManager::getInstance().cancel(player);
    mc_utils::sendText(out, "已取消新建领地"_trf(player));
};


static auto const Buy = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    BuyLandGui::impl(player);
};

static auto const Reload = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    if (Config::tryLoad()) {
        mod::ModEntry::getInstance().onConfigReload();
        mc_utils::sendText(out, "领地系统配置已重新加载"_tr());
    } else {
        mc_utils::sendText(out, "领地系统配置加载失败，请检查配置文件"_tr());
    }
};


enum class DrawType : int { Disable = 0, NearLand, CurrentLand };
struct DrawParam {
    DrawType type;
};
static auto const Draw = [](CommandOrigin const& ori, CommandOutput& out, DrawParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);

    auto& player = *static_cast<Player*>(ori.getEntity());
    auto& db     = PLand::getInstance();
    auto  handle = DrawHandleManager::getInstance().getOrCreateHandle(player);

    switch (param.type) {
    case DrawType::Disable: {
        handle->removeLands();
        mc_utils::sendText(out, "领地绘制已关闭"_trf(player));
        break;
    }

    case DrawType::CurrentLand: {
        auto land = db.getLandAt(player.getPosition(), player.getDimensionId().id);
        if (!land) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(out, "您当前不在领地内"_trf(player));
            return;
        }
        handle->draw(land, mce::Color::GREEN());
        mc_utils::sendText(out, "已绘制领地"_trf(player));
        break;
    }

    case DrawType::NearLand: {
        auto lands = db.getLandAt(player.getPosition(), Config::cfg.land.drawRange, player.getDimensionId().id);
        for (auto& land : lands) handle->draw(land, mce::Color::WHITE());
        mc_utils::sendText(out, "已绘制附近 {} 个领地"_trf(player, lands.size()));
        break;
    }
    }
};


struct ImportParam {
    bool   clearDb;
    string relationship_file;
    string data_file;
};
static auto const Import = [](CommandOrigin const& ori, CommandOutput& out, ImportParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);

    if (!fs::exists(param.relationship_file)) {
        out.error("未找到 relationship.json 文件"_tr());
        return;
    }
    if (!fs::exists(param.data_file)) {
        out.error("未找到 data.json 文件"_tr());
        return;
    }
    if (fs::path(param.relationship_file).filename() != "relationship.json") {
        out.error("relationship.json 文件名错误"_tr());
        return;
    }

    if (iLandConverter(param.relationship_file, param.data_file, param.clearDb).execute()) {
        out.success("导入成功"_tr());
    } else {
        out.error("导入失败"_tr());
    }
};

static auto const SetLandTeleportPos = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    auto& db   = PLand::getInstance();
    auto  land = db.getLandAt(player.getPosition(), player.getDimensionId().id);
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(out, "您当前不在领地内"_trf(player));
        return;
    }

    auto uuid = player.getUuid().asString();
    if (!land->isLandOwner(uuid) && !db.isOperator(uuid)) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(out, "您不是领地主人，无法设置传送点"_trf(player));
        return;
    }
    land->mTeleportPos = player.getPosition();
};


static auto const SetLanguage = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    using ll::form::CustomForm;
    using ll::form::CustomFormResult;
    using ll::form::FormCancelReason;

    static std::vector<std::string> langs = {
        PlayerSettings::SERVER_LOCALE_CODE(),
        PlayerSettings::SYSTEM_LOCALE_CODE()
    };
    if (langs.size() == 2) {
        fs::path const& langDir = mod::ModEntry::getInstance().getSelf().getLangDir();
        for (auto const& lang : fs::directory_iterator(langDir)) {
            if (lang.path().extension() == ".json") {
                langs.push_back(lang.path().stem().string());
            }
        }
    }
    CustomForm fm(PLUGIN_NAME + ("| 选择语言"_trf(player)));
    fm.appendLabel("system: 为使用当前客户端语言\nserver: 为使用服务端语言"_trf(player));
    fm.appendLabel("当前使用语言包: {}"_trf(player, GetPlayerLocaleCodeFromSettings(player)));
    fm.appendDropdown("lang", "选择语言"_trf(player), langs);
    fm.sendTo(player, [](Player& pl, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        auto lang = std::get<std::string>(res->at("lang"));

        auto  uuid     = pl.getUuid().asString();
        auto& db       = PLand::getInstance();
        auto  settings = db.getPlayerSettings(uuid);
        if (!settings) {
            db.setPlayerSettings(uuid, PlayerSettings{});
            settings = db.getPlayerSettings(uuid);
        }

        settings->localeCode               = lang;
        GlobalPlayerLocaleCodeCached[uuid] = lang;
        mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "语言包已切换为: {}"_trf(pl, lang));
    });
};

static auto const This = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    auto land = PLand::getInstance().getLandAt(player.getPosition(), player.getDimensionId());
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Info>(player, "当前位置没有领地"_trf(player));
        return;
    }

    auto uuidStr = player.getUuid().asString();
    if (!land->isLandOwner(uuidStr) && !PLand::getInstance().isOperator(uuidStr)) {
        mc_utils::sendText<mc_utils::LogLevel::Info>(player, "当前位置不是你的领地"_trf(player));
        return;
    }

    LandManageGui::impl(player, land->getLandID());
};

}; // namespace Lambda


bool LandCommand::setup() {
    auto& cmd = ll::command::CommandRegistrar::getInstance().getOrCreateCommand("pland", "PLand 领地系统"_tr());

    // pland reload
    cmd.overload().text("reload").execute(Lambda::Reload);

    // pland 领地GUI
    cmd.overload().execute(Lambda::Root);

    // pland gui 领地管理GUI
    cmd.overload().text("gui").execute(Lambda::Root);

    // pland mgr 领地管理GUI
    cmd.overload().text("mgr").execute(Lambda::Mgr);

    // pland <op/deop> <player> 添加/移除管理员
    cmd.overload<Lambda::OperatorParam>().required("type").required("player").execute(Lambda::Operator);

    // pland list op
    cmd.overload().text("list").text("op").execute(Lambda::ListOperator);

    // pland new [NewType: type] 新建一个领地
    cmd.overload<Lambda::NewParam>().text("new").optional("type").execute(Lambda::New);

    // pland this 获取当前领地信息
    cmd.overload().text("this").execute(Lambda::This);

    // pland set <a/b> 选点a/b
    cmd.overload<Lambda::SetParam>().text("set").required("type").execute(Lambda::Set);

    // pland cancel 取消新建
    cmd.overload().text("cancel").execute(Lambda::Cancel);

    // pland buy 购买
    cmd.overload().text("buy").execute(Lambda::Buy);

    // pland draw <disable|near|current> 开启/关闭领地绘制
    if (Config::cfg.land.setupDrawCommand) {
        cmd.overload<Lambda::DrawParam>().text("draw").required("type").execute(Lambda::Draw);
    }

    // pland import <land_type> <clear_db> <Args...>
    cmd.overload<Lambda::ImportParam>()
        .text("import")
        .required("clearDb")
        .required("relationship_file")
        .required("data_file")
        .execute(Lambda::Import);

    // pland set teleport_pos 设置传送点
    cmd.overload().text("set").text("teleport_pos").execute(Lambda::SetLandTeleportPos);

    // pland set language 设置语言
    cmd.overload().text("set").text("language").execute(Lambda::SetLanguage);

#ifdef LD_DEVTOOL
    // pland devtool
    if (Config::cfg.internal.devTools) {
        cmd.overload().text("devtool").execute([](CommandOrigin const& ori, CommandOutput&) {
            if (ori.getOriginType() == CommandOriginType::DedicatedServer) {
                if (auto tool = devtool::DevToolAppManager::getInstance().getOrCreateApp()) {
                    tool->show();
                }
            }
        });
    }
#endif

    return true;
}


} // namespace land
