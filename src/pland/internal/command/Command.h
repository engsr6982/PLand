#pragma once

namespace ll::command {
class CommandHandle;
class CommandRegistrar;
}; // namespace ll::command

class CommandOrigin;
class CommandOutput;

namespace land::internal {

struct LandCommand {
    LandCommand() = delete;

    static bool setupAll();

    /// sub commands

    static void setupLeaseSubCommands(ll::command::CommandRegistrar& reg, ll::command::CommandHandle& h);

    /// heleprs

    static bool ensure_origin_console_or_player(CommandOrigin const& ori, CommandOutput& out);
    static bool ensure_admin(CommandOrigin const& ori, CommandOutput& out);
};


} // namespace land::internal