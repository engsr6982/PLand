#include "pland/internal/interceptor/EventInterceptor.h"
#include "pland/internal/interceptor/InterceptorConfig.h"
#include "pland/internal/interceptor/helper/EventTrace.h"
#include "pland/internal/interceptor/helper/InterceptorHelper.h"

#include <ll/api/event/EventBus.h>

#include "ila/event/minecraft/world/actor/ArmorStandSwapItemEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerAttackBlockEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerDropItemEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerEditSignEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerInteractEntityEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerOperatedItemFrameEvent.h"

#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"

namespace land::internal::interceptor {

void EventInterceptor::setupIlaPlayerListeners() {
    auto registry = &PLand::getInstance().getLandRegistry();
    auto bus      = &ll::event::EventBus::getInstance();

    registerListenerIf<&InterceptorConfig::Listeners::PlayerInteractEntityBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::PlayerInteractEntityBeforeEvent>(
            [registry](ila::mc::PlayerInteractEntityBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::PlayerInteractEntityBeforeEvent);

                auto& player = ev.self();
                auto& target = ev.target();

                TRACE_LOG("player={}, target={}", player.getRealName(), target.getTypeName());

                auto land = registry->getLandAt(target.getPosition(), target.getDimensionId());
                if (!hasRolePermission<&RolePerms::allowInteractEntity>(land, player.getUuid())) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::ArmorStandSwapItemBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::ArmorStandSwapItemBeforeEvent>(
            [registry](ila::mc::ArmorStandSwapItemBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::ArmorStandSwapItemBeforeEvent);

                auto&    player     = ev.player();
                auto&    armorStand = ev.self();
                BlockPos pos        = armorStand.getPosition();

                TRACE_LOG("player={}, armorStandPos={}", player.getRealName(), pos.toString());

                auto land = registry->getLandAt(armorStand.getPosition(), armorStand.getDimensionId());
                if (!hasRolePermission<&RolePerms::useArmorStand>(land, player.getUuid())) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::PlayerDropItemBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::PlayerDropItemBeforeEvent>(
            [registry](ila::mc::PlayerDropItemBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::PlayerDropItemBeforeEvent);

                auto& player = ev.self();

                if (!player.hasDimension()) {
                    TRACE_LOG("player has no dimension yet, skip PlayerDropItemBeforeEvent");
                    return;
                }

                TRACE_LOG("player={}", player.getRealName());

                auto land = registry->getLandAt(player.getPosition(), player.getDimensionId());
                if (!hasRolePermission<&RolePerms::allowDropItem>(land, player.getUuid())) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::PlayerOperatedItemFrameBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::PlayerOperatedItemFrameBeforeEvent>(
            [registry](ila::mc::PlayerOperatedItemFrameBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::PlayerOperatedItemFrameBeforeEvent);

                auto& player = ev.self();
                auto& pos    = ev.blockPos();

                TRACE_LOG("player={}, pos={}", player.getRealName(), pos.toString());

                auto land = registry->getLandAt(ev.blockPos(), player.getDimensionId());
                if (!hasRolePermission<&RolePerms::useItemFrame>(land, player.getUuid())) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::PlayerEditSignBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::PlayerEditSignBeforeEvent>(
            [registry](ila::mc::PlayerEditSignBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::PlayerEditSignBeforeEvent);

                auto& player = ev.self();
                auto& pos    = ev.pos();

                TRACE_LOG("player={}, pos={}", player.getRealName(), pos.toString());

                auto land = registry->getLandAt(pos, player.getDimensionId());
                if (!hasRolePermission<&RolePerms::editSign>(land, player.getUuid())) {
                    ev.cancel();
                }
            }
        );
    });
}

} // namespace land::internal::interceptor
