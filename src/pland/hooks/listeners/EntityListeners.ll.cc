
#include "pland/hooks/EventListener.h"
#include "pland/hooks/listeners/ListenerHelper.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/world/SpawnMobEvent.h"

#include "mc/server/ServerPlayer.h"

#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"


namespace land {

void EventListener::registerLLEntityListeners() {
    auto* db     = PLand::getInstance().getLandRegistry();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &land::PLand::getInstance().getSelf().getLogger();

    RegisterListenerIf(Config::cfg.listeners.SpawnedMobEvent, [&]() {
        return bus->emplaceListener<ll::event::SpawnedMobEvent>([db, logger](ll::event::SpawnedMobEvent& ev) {
            auto mob = ev.mob();
            if (!mob.has_value()) return;
            auto& pos = mob->getPosition();
            logger->debug("[SpawnedMob] {}", pos.toString());
            auto land = db->getLandAt(pos, mob->getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) return;
            auto const& tab       = land->getPermTable();
            bool        isMonster = mob->hasCategory(::ActorCategory::Monster) || mob->hasFamily("monster");
            if (isMonster) {
                if (!tab.allowMonsterSpawn) mob->despawn();
            } else {
                if (!tab.allowAnimalSpawn) mob->despawn();
            }
        });
    });
}

} // namespace land
