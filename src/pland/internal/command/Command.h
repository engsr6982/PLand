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
};


} // namespace land::internal