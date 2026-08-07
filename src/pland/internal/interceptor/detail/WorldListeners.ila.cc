#include "pland/internal/interceptor/EventInterceptor.h"
#include "pland/internal/interceptor/InterceptorConfig.h"
#include "pland/internal/interceptor/helper/EventTrace.h"
#include "pland/internal/interceptor/helper/InterceptorHelper.h"
#include "pland/service/LandHierarchyService.h"
#include "pland/service/ServiceLocator.h"

#include "ll/api/event/EventBus.h"

#include "ila/event/minecraft/world/ExplosionEvent.h"
#include "ila/event/minecraft/world/PistonPushEvent.h"
#include "ila/event/minecraft/world/RedstoneUpdateEvent.h"
#include "ila/event/minecraft/world/SculkBlockGrowthEvent.h"
#include "ila/event/minecraft/world/WitherDestroyEvent.h"
#include "ila/event/minecraft/world/level/block/BlockFallEvent.h"
#include "ila/event/minecraft/world/level/block/DragonEggBlockTeleportEvent.h"
#include "ila/event/minecraft/world/level/block/LiquidFlowEvent.h"
#include "ila/event/minecraft/world/level/block/MossGrowthEvent.h"
#include "ila/event/minecraft/world/level/block/SculkCatalystAbsorbExperienceEvent.h"
#include "ila/event/minecraft/world/level/block/SculkSpreadEvent.h"

#include "mc/world/level/Explosion.h"
#include "mc/world/phys/AABB.h"

namespace land::internal::interceptor {

void EventInterceptor::setupIlaWorldListeners() {
    auto registry = &PLand::getInstance().getLandRegistry();
    auto bus      = &ll::event::EventBus::getInstance();

    registerListenerIf<&InterceptorConfig::Listeners::ExplosionBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::ExplosionBeforeEvent>([registry](ila::mc::ExplosionBeforeEvent& ev) {
            TRACE_THIS_EVENT(ila::mc::ExplosionBeforeEvent);

            auto& explosion   = ev.explosion();
            auto& blockSource = explosion.mRegion;
            auto  centerPos   = BlockPos{explosion.mPos};
            auto  radius      = explosion.mRadius;
            auto  dimid       = blockSource.getDimensionId();

            TRACE_LOG("centerPos={}, radius={}", centerPos.toString(), radius);

            auto centerLand = registry->getLandAt(centerPos, dimid);
            if (!hasEnvironmentPermission<&EnvironmentPerms::allowExplode>(centerLand)) {
                ev.cancel();
                TRACE_LOG("center land does not allow explode");
                return; // 爆炸中心所在领地的权限具有决定性
            }

            auto touchedLands = registry->getLandAt(centerPos, (int)(radius + 1.0), dimid);
            if (centerLand) {
                // 如果中心领地允许爆炸，检查是否影响到其他禁止爆炸的、不相关的领地。
                auto& service    = PLand::getInstance().getServiceLocator().getLandHierarchyService();
                auto  centerRoot = service.getRoot(centerLand);
                for (auto const& touchedLand : touchedLands) {
                    if (service.getRoot(touchedLand) != centerRoot) {
                        if (!hasEnvironmentPermission<&EnvironmentPerms::allowExplode>(touchedLand)) {
                            TRACE_LOG("touched land does not allow explode");
                            ev.cancel();
                            return;
                        }
                    }
                }
            } else {
                // 情况：爆炸发生在领地外。
                // 如果影响到任何禁止爆炸的领地，则取消。
                for (auto const& touchedLand : touchedLands) {
                    if (!hasEnvironmentPermission<&EnvironmentPerms::allowExplode>(touchedLand)) {
                        TRACE_LOG("external land does not allow explode");
                        ev.cancel();
                        return;
                    }
                }
            }
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::PistonPushBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::PistonPushBeforeEvent>([registry](ila::mc::PistonPushBeforeEvent& ev) {
            auto& pistonPos = ev.pistonPos();
            auto& pushPos   = ev.pushPos();

            auto dimid      = ev.blockSource().getDimensionId();
            auto pistonLand = registry->getLandAt(pistonPos, dimid);
            auto pushLand   = registry->getLandAt(pushPos, dimid);

            // 由于活塞事件复杂，需要处理4种可能的情况
            // 内 => 内 / 外 => 内 / 内 => 外 / 外 => 外

            // 内 => 内
            if (pistonLand && pushLand) {
                if (pistonLand == pushLand) {
                    return; // 内 => 内
                }
                bool pistonAllow = hasEnvironmentPermission<&EnvironmentPerms::allowPistonPushOnBoundary>(pistonLand);
                bool pushAllow   = hasEnvironmentPermission<&EnvironmentPerms::allowPistonPushOnBoundary>(pushLand);
                if (pistonAllow && pushAllow) {
                    return; // 内A => 内B（跨领地）
                }
                ev.cancel();
                return;
            }

            // 外 => 内
            if (!pistonLand && pushLand) {
                bool pushAllow = hasEnvironmentPermission<&EnvironmentPerms::allowPistonPushOnBoundary>(pushLand);
                if (!pushAllow
                    && (pushLand->getAABB().isOnOuterBoundary(pistonPos)
                        || pushLand->getAABB().isOnInnerBoundary(pushPos))) {
                    ev.cancel(); // 外 => 内
                }
            }
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::RedstoneUpdateBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::RedstoneUpdateBeforeEvent>(
            [registry](ila::mc::RedstoneUpdateBeforeEvent& ev) {
                auto& blockSource = ev.blockSource();
                auto& blockPos    = ev.pos();

                auto land = registry->getLandAt(blockPos, blockSource.getDimensionId());
                if (!hasEnvironmentPermission<&EnvironmentPerms::allowRedstoneUpdate>(land)) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::BlockFallBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::BlockFallBeforeEvent>([registry](ila::mc::BlockFallBeforeEvent& ev) {
            auto& blockSource = ev.blockSource();
            auto& blockPos    = ev.pos();

            auto land = registry->getLandAt(blockPos, blockSource.getDimensionId());
            if (land && land->getAABB().isAboveLand(blockPos)
                && !hasEnvironmentPermission<&EnvironmentPerms::allowBlockFall>(land)) {
                ev.cancel();
            }
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::WitherDestroyBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::WitherDestroyBeforeEvent>(
            [registry](ila::mc::WitherDestroyBeforeEvent& ev) {
                TRACE_THIS_EVENT(ila::mc::WitherDestroyBeforeEvent);

                auto& aabb = ev.box();

                TRACE_LOG("aabb={}", aabb.toString());

                static constexpr float Offset = 1.0f; // 由于闭区间判定以及浮点数精度，需要额外偏移1个单位

                auto lands =
                    registry->getLandAt(aabb.min - Offset, aabb.max + Offset, ev.blockSource().getDimensionId());
                for (auto const& p : lands) {
                    if (!hasEnvironmentPermission<&EnvironmentPerms::allowWitherDestroy>(p)) {
                        ev.cancel();
                        break;
                    }
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::MossGrowthBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::MossGrowthBeforeEvent>([registry](ila::mc::MossGrowthBeforeEvent& ev) {
            auto& blockSource = ev.blockSource();
            auto& blockPos    = ev.pos();
            int   rx          = ev.xRadius();
            int   rz          = ev.zRadius();

            auto minPos = Vec3(blockPos.x - rx, blockPos.y - 1, blockPos.z - rz);
            auto maxPos = Vec3(blockPos.x + rx, blockPos.y + 1, blockPos.z + rz);

            auto lds = registry->getLandAt(minPos, maxPos, blockSource.getDimensionId());
            for (auto const& land : lds) {
                if (!hasEnvironmentPermission<&EnvironmentPerms::allowMossGrowth>(land)) {
                    ev.cancel();
                    return;
                }
            }
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::LiquidFlowBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::LiquidFlowBeforeEvent>([registry](ila::mc::LiquidFlowBeforeEvent& ev) {
            auto& blockSource = ev.blockSource();
            auto& fromPos     = ev.flowFromPos(); // 源头 (水流来的方向)
            auto& toPos       = ev.pos();         // 目标 (水流要去的地方)

            auto landTo = registry->getLandAt(toPos, blockSource.getDimensionId());
            if (landTo && !hasEnvironmentPermission<&EnvironmentPerms::allowLiquidFlow>(landTo)
                && landTo->getAABB().isOnOuterBoundary(fromPos) && landTo->getAABB().isOnInnerBoundary(toPos)) {
                ev.cancel();
            }
        });
    });

    registerListenerIf<&InterceptorConfig::Listeners::DragonEggBlockTeleportBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::DragonEggBlockTeleportBeforeEvent>(
            [registry](ila::mc::DragonEggBlockTeleportBeforeEvent& ev) {
                auto& blockSource = ev.blockSource();
                auto& blockPos    = ev.pos();

                auto land = registry->getLandAt(blockPos, blockSource.getDimensionId());
                if (!hasEnvironmentPermission<&EnvironmentPerms::allowDragonEggTeleport>(land)) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::SculkBlockGrowthBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::SculkBlockGrowthBeforeEvent>(
            [registry](ila::mc::SculkBlockGrowthBeforeEvent& ev) {
                auto& blockSource = ev.blockSource();
                auto& blockPos    = ev.pos();

                auto land = registry->getLandAt(blockPos, blockSource.getDimensionId());
                if (!hasEnvironmentPermission<&EnvironmentPerms::allowSculkBlockGrowth>(land)) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf<&InterceptorConfig::Listeners::SculkSpreadBeforeEvent>([bus, registry]() {
        return bus->emplaceListener<ila::mc::SculkSpreadBeforeEvent>([registry](ila::mc::SculkSpreadBeforeEvent& ev) {
            auto& blockSource = ev.blockSource();
            auto& fromPos     = ev.selfPos();
            auto& toPos       = ev.targetPos();

            auto sou = registry->getLandAt(fromPos, blockSource.getDimensionId());
            auto tar = registry->getLandAt(toPos, blockSource.getDimensionId());

            if (!hasEnvironmentPermission<&EnvironmentPerms::allowSculkSpread>(sou)
                || !hasEnvironmentPermission<&EnvironmentPerms::allowSculkSpread>(tar)) {
                ev.cancel();
            }
        });
    });
}

} // namespace land::internal::interceptor
