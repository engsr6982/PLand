
#include "pland/hooks/EventListener.h"
#include "pland/hooks/listeners/ListenerHelper.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/world/SpawnMobEvent.h"

#include "ila/event/minecraft/world/actor/ActorDestroyBlockEvent.h"
#include "ila/event/minecraft/world/actor/ActorRideEvent.h"
#include "ila/event/minecraft/world/actor/ActorTriggerPressurePlateEvent.h"
#include "ila/event/minecraft/world/actor/EndermanLeaveBlockEvent.h"
#include "ila/event/minecraft/world/actor/EndermanTakeBlockEvent.h"
#include "ila/event/minecraft/world/actor/MobHurtEffectEvent.h"
#include "ila/event/minecraft/world/actor/ProjectileCreateEvent.h"

#include "mc/server/ServerPlayer.h"

#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"


namespace land {

void EventListener::registerEntityListeners() {
    auto* db     = PLand::getInstance().getLandRegistry();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &land::PLand::getInstance().getSelf().getLogger();

    RegisterListenerIf(Config::cfg.listeners.ActorDestroyBlockEvent, [&]() {
        return bus->emplaceListener<ila::mc::ActorDestroyBlockEvent>([db, logger](ila::mc::ActorDestroyBlockEvent& ev) {
            auto& actor    = ev.self();
            auto& blockPos = ev.pos();
            logger->debug("[ActorDestroyBlock] Actor: {}, Pos: {}", actor.getTypeName(), blockPos.toString());
            auto land = db->getLandAt(blockPos, actor.getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) return;
            if (land->getPermTable().allowActorDestroy) return;
            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.EndermanLeaveBlockEvent, [&]() {
        return bus->emplaceListener<ila::mc::EndermanLeaveBlockBeforeEvent>(
            [db, logger](ila::mc::EndermanLeaveBlockBeforeEvent& ev) {
                auto& actor    = ev.self();
                auto& blockPos = ev.pos();
                logger->debug("[EndermanLeave] Actor: {}, Pos: {}", actor.getTypeName(), blockPos.toString());
                auto land = db->getLandAt(blockPos, actor.getDimensionId());
                if (PreCheckLandExistsAndPermission(land)) return;
                if (land->getPermTable().allowActorDestroy) return;
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.EndermanTakeBlockEvent, [&]() {
        return bus->emplaceListener<ila::mc::EndermanLeaveBlockBeforeEvent>(
            [db, logger](ila::mc::EndermanLeaveBlockBeforeEvent& ev) {
                auto& actor    = ev.self();
                auto& blockPos = ev.pos();
                logger->debug("[EndermanLeave] Actor: {}, Pos: {}", actor.getTypeName(), blockPos.toString());
                auto land = db->getLandAt(blockPos, actor.getDimensionId());
                if (PreCheckLandExistsAndPermission(land)) return;
                if (land->getPermTable().allowActorDestroy) return;
                ev.cancel();
            }
        );
    });

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

    RegisterListenerIf(Config::cfg.listeners.ActorRideBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::ActorRideBeforeEvent>([db, logger](ila::mc::ActorRideBeforeEvent& ev) {
            Actor& passenger = ev.self();
            Actor& target    = ev.target();
            logger->debug(
                "[ActorRide]: passenger: {}, target: {}",
                passenger.getActorIdentifier().mIdentifier.get(),
                target.getTypeName()
            );
            if (!passenger.isPlayer()) return;
            auto const& typeName = ev.target().getTypeName();
            auto        land     = db->getLandAt(target.getPosition(), target.getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) return;
            if (land) {
                auto& tab = land->getPermTable();
                if (typeName == "minecraft:minecart" || typeName == "minecraft:boat"
                    || typeName == "minecraft:chest_boat") {
                    if (tab.allowRideTrans) return;
                } else {
                    if (tab.allowRideEntity) return;
                }
            }
            if (passenger.isPlayer()) {
                auto player = passenger.getWeakEntity().tryUnwrap<Player>();
                if (player.has_value() && PreCheckLandExistsAndPermission(land, player->getUuid().asString())) {
                    return;
                }
            }
            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.MobHurtEffectBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::MobHurtEffectBeforeEvent>([db,
                                                                        logger](ila::mc::MobHurtEffectBeforeEvent& ev) {
            auto& hurtActor  = ev.self();
            auto  hurtSource = ev.source();
            if (!hurtSource) return;
            auto const hurtActorTypeName  = hurtActor.getTypeName();
            auto       hurtSourceTypeName = hurtSource->getTypeName();
            if (hurtSourceTypeName != "minecraft:player") return;
            auto land = db->getLandAt(hurtActor.getPosition(), hurtActor.getDimensionId());
            if (!land) return;
            auto& player = static_cast<Player&>(hurtSource.value());
            if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) return;
            auto const& tab = land->getPermTable();
            if (hurtActor.isPlayer()) {
                CANCEL_AND_RETURN_IF(!tab.allowPlayerDamage);
            } else if (Config::cfg.mob.hostileMobTypeNames.contains(hurtActorTypeName)) {
                CANCEL_AND_RETURN_IF(!tab.allowMonsterDamage);
            } else if (Config::cfg.mob.specialMobTypeNames.contains(hurtActorTypeName)) {
                CANCEL_AND_RETURN_IF(!tab.allowSpecialDamage);
            } else if (Config::cfg.mob.passiveMobTypeNames.contains(hurtActorTypeName)) {
                CANCEL_AND_RETURN_IF(!tab.allowPassiveDamage);
            } else if (Config::cfg.mob.customSpecialMobTypeNames.contains(hurtActorTypeName)) {
                CANCEL_AND_RETURN_IF(!tab.allowCustomSpecialDamage);
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.ActorTriggerPressurePlateBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::ActorTriggerPressurePlateBeforeEvent>(
            [db, logger](ila::mc::ActorTriggerPressurePlateBeforeEvent& ev) {
                logger->debug("[PressurePlateTrigger] pos: {}", ev.pos().toString());
                auto land = db->getLandAt(ev.pos(), ev.self().getDimensionId());
                if (land && land->getPermTable().usePressurePlate) return;
                if (PreCheckLandExistsAndPermission(land)) return;
                auto& entity = ev.self();
                if (entity.isPlayer()) {
                    auto pl = entity.getWeakEntity().tryUnwrap<Player>();
                    if (pl.has_value() && PreCheckLandExistsAndPermission(land, pl->getUuid().asString())) return;
                }
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.ProjectileCreateBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::ProjectileCreateBeforeEvent>(
            [db, logger](ila::mc::ProjectileCreateBeforeEvent& ev) {
                Actor& self = ev.self();
                auto&  type = ev.self().getTypeName();
                logger->debug("[ProjectileSpawn] type: {}", type);
                auto mob = self.getOwner();
                if (!mob) return;
                auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
                if (PreCheckLandExistsAndPermission(land)) return;
                if (self.getOwnerEntityType() == ActorType::Player) {
                    if (mob->isPlayer()) {
                        auto pl = mob->getWeakEntity().tryUnwrap<Player>();
                        if (pl.has_value() && PreCheckLandExistsAndPermission(land, pl->getUuid().asString())) return;
                    }
                }
                if (land) {
                    auto const& tab = land->getPermTable();
                    if (mob->isPlayer()) {
                        if (type == "minecraft:fishing_hook") {
                            CANCEL_AND_RETURN_IF(!tab.useFishingHook);
                        } else {
                            CANCEL_AND_RETURN_IF(!tab.allowProjectileCreate);
                        }
                    }
                }
            }
        );
    });
}

} // namespace land
