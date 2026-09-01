#include "pland/internal/interceptor/EventInterceptor.h"
#include "pland/internal/interceptor/InterceptorConfig.h"
#include "pland/internal/interceptor/helper/EventTrace.h"
#include "pland/internal/interceptor/helper/InterceptorHelper.h"

#include "ila/event/minecraft/world/actor/ActorDestroyBlockEvent.h"
#include "ila/event/minecraft/world/actor/ActorPickupItemEvent.h"
#include "ila/event/minecraft/world/actor/ActorRideEvent.h"
#include "ila/event/minecraft/world/actor/ActorTriggerPressurePlateEvent.h"
#include "ila/event/minecraft/world/actor/MobHurtEffectEvent.h"
#include "ila/event/minecraft/world/actor/MobPlaceBlockEvent.h"
#include "ila/event/minecraft/world/actor/MobTakeBlockEvent.h"
#include "ila/event/minecraft/world/actor/ProjectileCreateEvent.h"

#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/platform/UUID.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/player/Inventory.h"
#include "mc/world/actor/player/PlayerInventory.h"
#include "mc/world/actor/projectile/AbstractArrow.h"
#include "mc/world/actor/projectile/ThrownTrident.h"

#include <ll/api/event/EventBus.h>
#include <mc/deps/nbt/CompoundTag.h>


namespace land::internal::interceptor {

void EventInterceptor::setupIlaEntityListeners() {
    auto registry = &PLand::getInstance().getLandRegistry();
    auto bus      = &ll::event::EventBus::getInstance();

    registerListenerIf<&InterceptorConfig::Listeners::ActorDestroyBlockEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::ActorDestroyBlockEvent>([registry](ila::mc::ActorDestroyBlockEvent& ev) {
            TRACE_THIS_EVENT(ila::mc::ActorDestroyBlockEvent);

            auto& actor    = ev.self();
            auto& blockPos = ev.pos();

            TRACE_LOG("actor={}, pos={}", actor.getTypeName(), blockPos.toString());

            auto land = registry->getLandAt(blockPos, actor.getDimensionId());
            if (!hasEnvironmentPermission<&EnvironmentPerms::allowMobGrief>(land)) {
                ev.cancel();
            }
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::MobTakeBlockBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::MobTakeBlockBeforeEvent>([registry](ila::mc::MobTakeBlockBeforeEvent& ev) {
            TRACE_THIS_EVENT(ila::mc::MobTakeBlockBeforeEvent);

            auto& actor    = ev.self();
            auto& blockPos = ev.pos();

            TRACE_LOG("actor={}, pos={}", actor.getTypeName(), blockPos.toString());

            auto land = registry->getLandAt(blockPos, actor.getDimensionId());
            if (!hasEnvironmentPermission<&EnvironmentPerms::allowMobGrief>(land)) {
                ev.cancel();
            }
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::MobPlaceBlockBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::MobPlaceBlockBeforeEvent>(
            [registry](ila::mc::MobPlaceBlockBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::MobPlaceBlockBeforeEvent);

                auto& actor    = ev.self();
                auto& blockPos = ev.pos();

                TRACE_LOG("actor={}, pos={}", actor.getTypeName(), blockPos.toString());

                auto land = registry->getLandAt(blockPos, actor.getDimensionId());
                if (!hasEnvironmentPermission<&EnvironmentPerms::allowMobGrief>(land)) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::ActorPickupItemBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::ActorPickupItemBeforeEvent>(
            [registry](ila::mc::ActorPickupItemBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::ActorPickupItemBeforeEvent);

                auto& actor = ev.self();
                auto& pos   = actor.getPosition();

                TRACE_LOG("actor={}, pos={}", actor.getTypeName(), pos.toString());

                auto land = registry->getLandAt(pos, actor.getDimensionId());
                if (!hasEnvironmentPermission<&EnvironmentPerms::allowMobGrief>(land)) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::ActorRideBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::ActorRideBeforeEvent>([registry](ila::mc::ActorRideBeforeEvent& ev) {
            TRACE_THIS_EVENT(ila::mc::ActorRideBeforeEvent);

            Actor& passenger = ev.self();
            Actor& target    = ev.target();

            if (passenger.getEntityTypeId() != ActorType::Player) {
                TRACE_LOG("passenger is not player");
                return;
            }
            if (target.hasCategory(ActorCategory::Ridable)) {
                TRACE_LOG("target is rideable");
                return;
            }

            TRACE_LOG(
                "passenger: {}, target: {}",
                passenger.getActorIdentifier().mIdentifier.get(),
                target.getTypeName()
            );
            auto& player = static_cast<Player&>(passenger);

            auto land = registry->getLandAt(target.getPosition(), target.getDimensionId());
            if (target.hasCategory(ActorCategory::BoatRideable) || target.hasCategory(ActorCategory::MinecartRidable)) {
                if (hasRolePermission<&RolePerms::allowRideTrans>(land, player.getUuid())) return;
            } else {
                if (hasRolePermission<&RolePerms::allowRideEntity>(land, player.getUuid())) return;
            }
            ev.cancel();
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::MobHurtEffectBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::MobHurtEffectBeforeEvent>(
            [registry](ila::mc::MobHurtEffectBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::MobHurtEffectBeforeEvent);

                auto& actor       = ev.self();
                auto  sourceActor = ev.source();

                if (!sourceActor || sourceActor->getEntityTypeId() != ActorType::Player) {
                    TRACE_LOG("source is not player");
                    return;
                }
                auto& player = static_cast<Player&>(sourceActor.value());

                auto uuid = player.getUuid();
                auto land = registry->getLandAt(actor.getPosition(), actor.getDimensionId());
                if (hasPrivilege(land, uuid)) return;

                if (actor.getEntityTypeId() == ActorType::Player) {
                    if (!hasMemberOrGuestPermission<&RolePerms::allowPvP>(land, uuid)) {
                        ev.cancel();
                        return;
                    }
                }

                HashedString typeName{actor.getTypeName()};

                auto category = InterceptorConfig::lookupMobDynamicCategory(typeName);
                switch (category) {
                case InterceptorConfig::MobRecordCategory::Hostile:
                    if (!hasMemberOrGuestPermission<&RolePerms::allowHostileDamage>(land, uuid)) {
                        ev.cancel();
                    }
                    break;
                case InterceptorConfig::MobRecordCategory::Friendly:
                    if (!hasMemberOrGuestPermission<&RolePerms::allowFriendlyDamage>(land, uuid)) {
                        ev.cancel();
                    }
                    break;
                case InterceptorConfig::MobRecordCategory::SpecialEntity:
                    if (!hasMemberOrGuestPermission<&RolePerms::allowSpecialEntityDamage>(land, uuid)) {
                        ev.cancel();
                    }
                    break;
                case InterceptorConfig::MobRecordCategory::Undefined:
                    break;
                };
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::ActorTriggerPressurePlateBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::ActorTriggerPressurePlateBeforeEvent>(
            [registry](ila::mc::ActorTriggerPressurePlateBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::ActorTriggerPressurePlateBeforeEvent);

                auto& actor    = ev.self();
                auto& blockPos = ev.pos();
                auto  isPlayer = actor.getEntityTypeId() == ActorType::Player;

                TRACE_LOG("pos={}, isPlayer={}", blockPos.toString(), isPlayer);

                auto land = registry->getLandAt(ev.pos(), actor.getDimensionId());
                if (isPlayer) {
                    auto& player = static_cast<Player&>(actor);
                    if (!hasRolePermission<&RolePerms::usePressurePlate>(land, player.getUuid())) {
                        ev.cancel();
                    }
                } else {
                    if (!hasGuestPermission<&RolePerms::usePressurePlate>(land)) {
                        ev.cancel();
                    }
                }
            }
        );
    });
}

} // namespace land::internal::interceptor
