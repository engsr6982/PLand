#include "TestMain.h"
#include "mc/world/actor/player/Player.h"
#include "pland/gui/form/PaginatedSimpleForm.h"
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/command/Overload.h>
#include <mc/server/commands/CommandOutput.h>

namespace test {


void TestMain::_setupPaginationFormTest() {
    ll::command::CommandRegistrar::getInstance()
        .getOrCreateCommand("testl")
        .overload()
        .text("pagination")
        .execute([](CommandOrigin const& origin, CommandOutput& output) {
            if (origin.getOriginType() != CommandOriginType::Player) {
                output.error("This command can only be run by a player");
                return;
            }
            auto& player = *static_cast<Player*>(origin.getEntity());

            auto fm = land::PaginatedSimpleFormFactory{"测试表单", ""};

            for (int i = 0; i < 16; i++) {
                fm.appendButton("按钮 #" + i, [i](Player& self) { self.sendMessage("你点击了按钮 #" + i); });
            }

            fm.buildAndSendTo(player);
        });
}


} // namespace test
