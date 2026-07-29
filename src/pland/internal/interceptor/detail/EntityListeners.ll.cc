#include "pland/internal/interceptor/EventInterceptor.h"
#include "pland/internal/interceptor/InterceptorConfig.h"
#include "pland/internal/interceptor/helper/EventTrace.h"
#include "pland/internal/interceptor/helper/InterceptorHelper.h"

#include "mc/server/ServerPlayer.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/level/Level.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/entity/ActorHurtEvent.h"
#include "ll/api/event/world/SpawnMobEvent.h"


namespace land::internal::interceptor {

void EventInterceptor::setupLLEntityListeners() {
    auto& config   = InterceptorConfig::cfg.listeners;
    auto  registry = &PLand::getInstance().getLandRegistry();
    auto  bus      = &ll::event::EventBus::getInstance();

    registerListenerIf(config.SpawnedMobEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::SpawnedMobEvent>([registry](ll::event::SpawnedMobEvent& ev) {
            TRACE_THIS_EVENT(ll::event::SpawnedMobEvent);

            auto mob = ev.mob();
            if (!mob) {
                return; // 因为事件是拦截特定类型实体生成，如果实体不存在这里直接跳过
            }

            auto     spawner = ev.spawner();
            BlockPos pos     = ev.pos();

            TRACE_LOG(
                "spawner={}, mob={}",
                spawner ? spawner->getTypeName() : "null",
                mob ? mob->getTypeName() : "null"
            );

            auto land = registry->getLandAt(pos, mob->getDimensionId());

            bool allowMonster = hasEnvironmentPermission<&EnvironmentPerms::allowMonsterSpawn>(land);
            bool allowAnimal  = hasEnvironmentPermission<&EnvironmentPerms::allowAnimalSpawn>(land);

            bool isMonster = mob->hasCategory(::ActorCategory::Monster) || mob->hasFamily("monster");
            if ((isMonster && !allowMonster) || (!isMonster && !allowAnimal)) {
                mob->despawn();
            }
        });
    });

    registerListenerIf(config.ActorHurtEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::ActorHurtEvent>([registry](ll::event::ActorHurtEvent& ev) {
            TRACE_THIS_EVENT(ll::event::ActorHurtEvent);

            auto& actor  = ev.self();
            auto& source = ev.source();

            if (source.getEntityType() != ActorType::Player) {
                TRACE_LOG("source is not player");
                return;
            }
            auto player = actor.getLevel().getPlayer(source.getEntityUniqueID());
            if (!player) {
                TRACE_LOG("source player not found");
                return;
            }

            auto& uuid = player->getUuid();
            auto  land = registry->getLandAt(actor.getPosition(), actor.getDimensionId());
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
        });
    });
}

} // namespace land::internal::interceptor
