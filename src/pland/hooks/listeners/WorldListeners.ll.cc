
#include "pland/hooks/EventListener.h"
#include "pland/hooks/listeners/ListenerHelper.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/world/FireSpreadEvent.h"

#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"

namespace land {

void EventListener::registerLLWorldListeners() {
    auto* db     = PLand::getInstance().getLandRegistry();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &land::PLand::getInstance().getSelf().getLogger();

    RegisterListenerIf(Config::cfg.listeners.FireSpreadEvent, [&]() {
        return bus->emplaceListener<ll::event::FireSpreadEvent>([db](ll::event::FireSpreadEvent& ev) {
            auto& pos  = ev.pos();
            auto  land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (PreCheckLandExistsAndPermission(land) || (land && land->getPermTable().allowFireSpread)) {
                return;
            }
            ev.cancel();
        });
    });
}

} // namespace land
