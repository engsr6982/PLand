
#include "pland/hooks/EventListener.h"
#include "pland/hooks/listeners/ListenerHelper.h"

#include "ll/api/event/EventBus.h"

#include "ila/event/minecraft/world/ExplosionEvent.h"
#include "ila/event/minecraft/world/PistonPushEvent.h"
#include "ila/event/minecraft/world/RedstoneUpdateEvent.h"
#include "ila/event/minecraft/world/SculkBlockGrowthEvent.h"
#include "ila/event/minecraft/world/WitherDestroyEvent.h"
#include "ila/event/minecraft/world/level/block/BlockFallEvent.h"
#include "ila/event/minecraft/world/level/block/DragonEggBlockTeleportEvent.h"
#include "ila/event/minecraft/world/level/block/FarmDecayEvent.h"
#include "ila/event/minecraft/world/level/block/LiquidTryFlowEvent.h"
#include "ila/event/minecraft/world/level/block/MossGrowthEvent.h"
#include "ila/event/minecraft/world/level/block/SculkCatalystAbsorbExperienceEvent.h"
#include "ila/event/minecraft/world/level/block/SculkSpreadEvent.h"

#include "mc/world/level/Explosion.h"
#include "mc/world/phys/AABB.h"

#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"

namespace land {

void EventListener::registerILAWorldListeners() {
    auto* db     = PLand::getInstance().getLandRegistry();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &land::PLand::getInstance().getSelf().getLogger();


    RegisterListenerIf(Config::cfg.listeners.ExplosionBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::ExplosionBeforeEvent>([db, logger](ila::mc::ExplosionBeforeEvent& ev) {
            logger->debug("[Explode] Pos: {}", ev.explosion().mPos->toString());
            auto       explosionPos = BlockPos{ev.explosion().mPos};
            auto       dimid        = ev.blockSource().getDimensionId();
            SharedLand centerLand   = db->getLandAt(explosionPos, dimid);

            if (centerLand) {
                // 规则一：爆炸中心所在领地的权限具有决定性。
                if (!centerLand->getPermTable().allowExplode) {
                    ev.cancel();
                    return;
                }

                // 规则二：如果中心领地允许爆炸，检查是否影响到其他禁止爆炸的、不相关的领地。
                auto touchedLands = db->getLandAt(explosionPos, (int)(ev.explosion().mRadius + 1.0), dimid);
                auto centerRoot   = centerLand->getRootLand();
                for (auto const& touchedLand : touchedLands) {
                    if (touchedLand->getRootLand() != centerRoot) {
                        if (!touchedLand->getPermTable().allowExplode) {
                            ev.cancel();
                            return;
                        }
                    }
                }
            } else {
                // 情况：爆炸发生在领地外。
                // 如果影响到任何禁止爆炸的领地，则取消。
                auto touchedLands = db->getLandAt(explosionPos, (int)(ev.explosion().mRadius + 1.0), dimid);
                for (auto const& touchedLand : touchedLands) {
                    if (!touchedLand->getPermTable().allowExplode) {
                        ev.cancel();
                        return;
                    }
                }
            }
        });
    });


    RegisterListenerIf(Config::cfg.listeners.FarmDecayBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::FarmDecayBeforeEvent>([db, logger](ila::mc::FarmDecayBeforeEvent& ev) {
            logger->debug("[FarmDecay] Pos: {}", ev.pos().toString());
            auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
            if (PreCheckLandExistsAndPermission(land) || (land && land->getPermTable().allowFarmDecay)) return;
            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PistonPushBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PistonPushBeforeEvent>([db, logger](ila::mc::PistonPushBeforeEvent& ev) {
            auto const& piston     = ev.pistonPos();
            auto const& push       = ev.pushPos();
            auto const  dimid      = ev.blockSource().getDimensionId();
            auto        pistonLand = db->getLandAt(piston, dimid);
            auto        pushLand   = db->getLandAt(push, dimid);
            if (pistonLand && pushLand) {
                if (pistonLand == pushLand
                    || (pistonLand->getPermTable().allowPistonPushOnBoundary
                        && pushLand->getPermTable().allowPistonPushOnBoundary)) {
                    return;
                }
                ev.cancel();
            } else if (!pistonLand && pushLand) {
                if (!pushLand->getPermTable().allowPistonPushOnBoundary
                    && (pushLand->getAABB().isOnOuterBoundary(piston) || pushLand->getAABB().isOnInnerBoundary(push))) {
                    ev.cancel();
                }
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.RedstoneUpdateBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::RedstoneUpdateBeforeEvent>(
            [db, logger](ila::mc::RedstoneUpdateBeforeEvent& ev) {
                auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
                if (PreCheckLandExistsAndPermission(land) || (land && land->getPermTable().allowRedstoneUpdate)) return;
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.BlockFallBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::BlockFallBeforeEvent>([db, logger](ila::mc::BlockFallBeforeEvent& ev) {
            auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
            if (land) {
                auto const& tab = land->getPermTable();
                CANCEL_AND_RETURN_IF(!tab.allowBlockFall);
                if (land->getAABB().isAboveLand(ev.pos()) && !tab.allowBlockFall) {
                    CANCEL_EVENT_AND_RETURN
                }
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.WitherDestroyBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::WitherDestroyBeforeEvent>([db,
                                                                        logger](ila::mc::WitherDestroyBeforeEvent& ev) {
            auto& aabb  = ev.box();
            auto  lands = db->getLandAt(aabb.min, aabb.max, ev.blockSource().getDimensionId());
            for (auto const& p : lands) {
                if (!p->getPermTable().allowWitherDestroy) {
                    ev.cancel();
                    break;
                }
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.MossGrowthBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::MossGrowthBeforeEvent>([db, logger](ila::mc::MossGrowthBeforeEvent& ev) {
            auto const& pos  = ev.pos();
            auto        land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (!land || land->getPermTable().useBoneMeal) return;
            auto lds = db->getLandAt(pos - 9, pos + 9, ev.blockSource().getDimensionId());
            for (auto const& p : lds) {
                if (p->getPermTable().useBoneMeal) return;
            }
            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.LiquidTryFlowBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::LiquidTryFlowBeforeEvent>([db,
                                                                        logger](ila::mc::LiquidTryFlowBeforeEvent& ev) {
            auto& sou    = ev.flowFromPos();
            auto& to     = ev.pos();
            auto  landTo = db->getLandAt(to, ev.blockSource().getDimensionId());
            if (landTo && !landTo->getPermTable().allowLiquidFlow && landTo->getAABB().isOnOuterBoundary(sou)
                && landTo->getAABB().isOnInnerBoundary(to)) {
                ev.cancel();
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.DragonEggBlockTeleportBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::DragonEggBlockTeleportBeforeEvent>(
            [db, logger](ila::mc::DragonEggBlockTeleportBeforeEvent& ev) {
                auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
                if (land && !land->getPermTable().allowAttackDragonEgg) {
                    ev.cancel();
                }
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.SculkBlockGrowthBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::SculkBlockGrowthBeforeEvent>(
            [db, logger](ila::mc::SculkBlockGrowthBeforeEvent& ev) {
                auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
                if (land && !land->getPermTable().allowSculkBlockGrowth) {
                    ev.cancel();
                }
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.SculkSpreadBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::SculkSpreadBeforeEvent>([db, logger](ila::mc::SculkSpreadBeforeEvent& ev) {
            auto sou = db->getLandAt(ev.selfPos(), ev.blockSource().getDimensionId());
            auto tar = db->getLandAt(ev.targetPos(), ev.blockSource().getDimensionId());
            if (!sou && tar) {
                ev.cancel();
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.SculkCatalystAbsorbExperienceBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::SculkCatalystAbsorbExperienceBeforeEvent>(
            [db, logger](ila::mc::SculkCatalystAbsorbExperienceBeforeEvent& ev) {
                auto& actor  = ev.actor();
                auto& region = actor.getDimensionBlockSource();
                auto  pos    = actor.getBlockPosCurrentlyStandingOn(&actor);
                auto  cur    = db->getLandAt(pos, region.getDimensionId());
                auto  lds    = db->getLandAt(pos - 9, pos + 9, region.getDimensionId());
                if ((cur && lds.size() == 1) || (!cur && lds.empty())) return;
                ev.cancel();
            }
        );
    });
}

} // namespace land
