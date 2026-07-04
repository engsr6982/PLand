#include "OnlinePlayerPicker.h"
#include "pland/Global.h"
#include "pland/gui/utils/BackUtils.h"

#include "ll/api/form/SimpleForm.h"

#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"


namespace land::gui {

using ll::form::SimpleForm;

void OnlinePlayerPicker::sendTo(Player& player, Callback const& callback, BackTo backTo, bool includeSimulatedPlayers) {
    SimpleForm f{"[PLand] | 玩家选择器"_trl(player.getLocaleCode())};

    if (backTo) {
        back_utils::injectBackButton(f, std::move(backTo));
    }

    player.getLevel().forEachPlayer([&f, callback, includeSimulatedPlayers](Player& target) {
        if (target.isSimulatedPlayer() && !includeSimulatedPlayers) {
            return true; // skip
        }
        f.appendButton(target.getRealName(), [weak = target.getEntityContext().getWeakRef(), callback](Player& player) {
            if (auto target = weak.tryUnwrap<Mob>(); target && target->getEntityTypeId() == ActorType::Player) {
                callback(player, *static_cast<Player*>(target.as_ptr()));
            }
        });
        return true;
    });

    f.sendTo(player);
}


} // namespace land::gui
