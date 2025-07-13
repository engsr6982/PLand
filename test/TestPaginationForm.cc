#include "TestMain.h"
#include "mc/world/actor/player/Player.h"
#include "pland/gui/form/PaginatedSimpleForm.h"
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/command/Overload.h>
#include <mc/server/commands/CommandOutput.h>

namespace test {

struct Number {
    int value = 16;
};

void TestMain::_setupPaginationFormTest() {
    ll::command::CommandRegistrar::getInstance()
        .getOrCreateCommand("testl")
        .overload<Number>()
        .text("pagination")
        .optional("value")
        .execute([](CommandOrigin const& origin, CommandOutput& output, Number const& value) {
            if (origin.getOriginType() != CommandOriginType::Player) {
                output.error("This command can only be run by a player");
                return;
            }
            auto& player = *static_cast<Player*>(origin.getEntity());

            auto fm = land::PaginatedSimpleFormFactory{"测试表单", {}};

            for (int i = 1; i <= value.value; i++) {
                auto stri = std::to_string(i);
                fm.appendButton("按钮 #" + stri, [stri](Player& self) { self.sendMessage("你点击了按钮 #" + stri); });
            }

            fm.buildAndSendTo(player);
        });
}


} // namespace test
