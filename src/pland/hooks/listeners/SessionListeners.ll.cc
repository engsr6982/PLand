
#include "pland/hooks/EventListener.h"
#include "pland/hooks/listeners/ListenerHelper.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"

#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/infra/DrawHandleManager.h"
#include "pland/land/LandRegistry.h"
#include "pland/land/LandScheduler.h"
#include "pland/selector/SelectorManager.h"

namespace land {

void EventListener::registerLLSessionListeners() {
    auto* db     = PLand::getInstance().getLandRegistry();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &land::PLand::getInstance().getSelf().getLogger();

    // PlayerJoin and PlayerDisconnect are fundamental and not behind a config flag.
    mListenerPtrs.push_back(bus->emplaceListener<ll::event::PlayerJoinEvent>([db,
                                                                              logger](ll::event::PlayerJoinEvent& ev) {
        if (ev.self().isSimulatedPlayer()) return;
        if (!db->hasPlayerSettings(ev.self().getUuid().asString())) {
            db->setPlayerSettings(ev.self().getUuid().asString(), PlayerSettings{}); // 新玩家
        }

        auto lands = db->getLands(ev.self().getXuid()); // xuid 查询
        if (!lands.empty()) {
            logger->info("Update land owner data from xuid to uuid for player {}", ev.self().getRealName());
            auto uuid = ev.self().getUuid().asString();
            for (auto& land : lands) {
                land->updateXUIDToUUID(uuid);
            }
        }
    }));
    mListenerPtrs.push_back(
        bus->emplaceListener<ll::event::PlayerDisconnectEvent>([logger](ll::event::PlayerDisconnectEvent& ev) {
            auto& player = ev.self();
            if (player.isSimulatedPlayer()) return;
            logger->debug("Player {} disconnect, remove all resources");

            auto& uuid    = player.getUuid();
            auto  uuidStr = uuid.asString();

            GlobalPlayerLocaleCodeCached.erase(uuidStr);
            land::PLand::getInstance().getSelectorManager()->stopSelection(uuid);
            DrawHandleManager::getInstance().removeHandle(player);
        })
    );
}

} // namespace land
