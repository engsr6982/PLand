#include "TestMain.h"
#include "mc/world/actor/player/Player.h"
#include "pland/gui/common/ChooseLandAdvancedUtilGUI.h"
#include "pland/land/Land.h"
#include "pland/land/LandRegistry.h"
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/command/Overload.h>
#include <mc/server/commands/CommandOutput.h>


namespace test {


void TestMain::_setupChooseLandAdvancedUtilGUITest() {
    ll::command::CommandRegistrar::getInstance()
        .getOrCreateCommand("testl")
        .overload()
        .text("choose_land_adv")
        .execute([](CommandOrigin const& origin, CommandOutput& output) {
            if (origin.getOriginType() != CommandOriginType::Player) {
                output.error("This command can only be run by a player");
                return;
            }
            auto& player = *static_cast<Player*>(origin.getEntity());

            auto lands = land::LandRegistry::getInstance().getLands();

            land::ChooseLandAdvancedUtilGUI::sendTo(player, lands, [](Player& self, land::SharedLand land) {
                self.sendMessage("选择领地: " + land->getName() + "ID: " + std::to_string(land->getId()));
            });
        });
}


} // namespace test