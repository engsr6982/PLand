#pragma once
#include "fmt/format.h"
#include "ll/api/service/Bedrock.h"
#include "mc/_HeaderOutputPredefine.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/core/utility/MCRESULT.h"
#include "mc/external/render_dragon/frame_renderer/CommandContext.h"
#include "mc/locale/I18n.h"
#include "mc/locale/Localization.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/commands/CommandBlockNameResult.h"
#include "mc/server/commands/CommandContext.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandOutputType.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandVersion.h"
#include "mc/server/commands/CurrentCmdVersion.h"
#include "mc/server/commands/MinecraftCommands.h"
#include "mc/server/commands/PlayerCommandOrigin.h"
#include "mc/server/commands/ServerCommandOrigin.h"
#include "mc/world/Minecraft.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/dimension/Dimension.h"
#include <ll/api/service/Bedrock.h>
#include <ll/api/service/ServerInfo.h>
#include <ll/api/service/Service.h>
#include <ll/api/service/ServiceManager.h>
#include <mc/deps/core/utility/optional_ref.h>
#include <mc/server/commands/Command.h>
#include <mc/server/commands/CommandContext.h>
#include <mc/server/commands/MinecraftCommands.h>
#include <mc/server/commands/PlayerCommandOrigin.h>
#include <mc/world/Minecraft.h>
#include <mc/world/actor/player/Player.h>
#include <memory>
#include <string>


namespace land::mc {

[[nodiscard]] inline Block const& getBlock(BlockPos& bp, int dimid) {
    return ll::service::getLevel()->getDimension(dimid)->getBlockSourceFromMainChunkSource().getBlock(bp);
}

inline void executeCommand(const std::string& cmd, Player* player = nullptr) {
    if (player) {
        // player
        CommandContext ctx = CommandContext(
            cmd,
            std::make_unique<PlayerCommandOrigin>(PlayerCommandOrigin(*player)),
            CommandVersion::CurrentVersion()
        );
        ll::service::getMinecraft()->getCommands().executeCommand(ctx, true);
    } else {
        // console
        CommandContext ctx = CommandContext(
            cmd,
            std::make_unique<ServerCommandOrigin>(
                "Server",
                ll::service::getLevel()->asServer(),
                CommandPermissionLevel::Owner,
                0
            ),
            CommandVersion::CurrentVersion()
        );
        ll::service::getMinecraft()->getCommands().executeCommand(ctx, true);
    }
}
[[nodiscard]] inline std::pair<bool, std::string> executeCommandEx(const std::string& cmd) {
    std::pair<bool, std::string> result;
    auto                         origin =
        ServerCommandOrigin("Server", ll::service::getLevel()->asServer(), CommandPermissionLevel::Internal, 0);
    auto command = ll::service::getMinecraft()->getCommands().compileCommand(
        cmd.c_str(),
        origin,
        (CurrentCmdVersion)CommandVersion::CurrentVersion(),
        [&](std::string const& err) { result.second.append(err).append("\n"); }
    );
    if (command) {
        CommandOutput output(CommandOutputType::AllOutput);
        command->run(origin, output);
        for (auto& msg : output.getMessages()) {
            std::string temp;
            getI18n().getCurrentLanguage()->get(msg.getMessageId(), temp, msg.getParams());
            result.second += temp.append("\n");
        }
        if (result.second.ends_with('\n')) {
            result.second.pop_back();
        }
        result.first = output.getSuccessCount() ? true : false;
        return result;
    }
    if (result.second.ends_with('\n')) {
        result.second.pop_back();
    }
    result.first = false;
    return result;
}

[[nodiscard]] inline BlockPos face2Pos(BlockPos const& sour, uchar face) {
    BlockPos dest = sour;
    switch (face) {
    case 0:
        --dest.y; // 下
        break;
    case 1:
        ++dest.y; // 上
        break;
    case 2:
        --dest.z; // 北
        break;
    case 3:
        ++dest.z; // 南
        break;
    case 4:
        --dest.x; // 西
        break;
    case 5:
        ++dest.x; // 东
        break;
    }
    return dest;
}


// Template function sendText, usage: sendText() or sendText<LogLevel::Success>().
enum class LogLevel : int { Normal = -1, Debug = 0, Info = 1, Warn = 2, Error = 3, Fatal = 4, Success = 5 };
inline static std::unordered_map<LogLevel, std::string> Color = {
    { LogLevel::Normal, "§b"},
    {  LogLevel::Debug, "§7"},
    {   LogLevel::Info, "§r"},
    {   LogLevel::Warn, "§e"},
    {  LogLevel::Error, "§c"},
    {  LogLevel::Fatal, "§4"},
    {LogLevel::Success, "§a"}
};

template <typename... Args>
[[nodiscard]] inline std::string format(const std::string& fmt, Args... args) {
    try {
        return fmt::vformat(fmt, fmt::make_format_args(args...));
    } catch (...) {
        return fmt;
    }
}

template <LogLevel type = LogLevel::Normal, typename... Args>
inline void sendText(Player& player, const std::string& fmt, Args&&... args) {
    player.sendMessage(format(PLUGIN_NAME + Color[type] + fmt, args...));
}
template <LogLevel type = LogLevel::Normal, typename... Args>
inline void sendText(CommandOutput& output, const std::string& fmt, Args&&... args) {
    if constexpr (type == LogLevel::Error || type == LogLevel::Fatal) {
        output.error(format(PLUGIN_NAME + Color[type] + fmt, args...));
    } else {
        output.success(format(PLUGIN_NAME + Color[type] + fmt, args...));
    }
}
template <LogLevel type = LogLevel::Normal, typename... Args>
inline void sendText(Player* player, const std::string& fmt, Args&&... args) {
    if (player) {
        return sendText<type>(*player, fmt, args...);
    } else {
        std::runtime_error("Failed in sendText: player is nullptr");
    }
}
template <LogLevel type = LogLevel::Normal, typename... Args>
inline void sendText(const std::string& realName, const std::string& fmt, Args&&... args) {
    auto level = ll::service::getLevel();
    if (level.has_value()) {
        return sendText<type>(level->getPlayer(realName), fmt, args...);
    } else {
        std::runtime_error("Failed in sendText: level is nullptr");
    }
}


} // namespace land::mc