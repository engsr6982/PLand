
#include "pland/hooks/EventListener.h"
#include "pland/hooks/listeners/ListenerHelper.h"

#include "ll/api/event/EventBus.h"

#include "ila/event/minecraft/world/actor/ArmorStandSwapItemEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerAttackBlockEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerDropItemEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerEditSignEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerInteractEntityEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerOperatedItemFrameEvent.h"

#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"

#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"


namespace land {

void EventListener::registerILAPlayerListeners() {
    auto* db     = PLand::getInstance().getLandRegistry();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &land::PLand::getInstance().getSelf().getLogger();

    RegisterListenerIf(Config::cfg.listeners.PlayerInteractEntityBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerInteractEntityBeforeEvent>(
            [db, logger](ila::mc::PlayerInteractEntityBeforeEvent& ev) {
                logger->debug("[交互实体] name: {}", ev.self().getRealName());
                auto& entity = ev.target();
                auto  land   = db->getLandAt(entity.getPosition(), ev.self().getDimensionId());
                if (PreCheckLandExistsAndPermission(land, ev.self().getUuid().asString())) return;
                if (land->getPermTable().allowInteractEntity) return;
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerAttackBlockBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerAttackBlockBeforeEvent>([db, logger](
                                                                               ila::mc::PlayerAttackBlockBeforeEvent& ev
                                                                           ) {
            auto& self = ev.self();
            auto& pos  = ev.pos();
            logger->debug("[AttackBlock] {}", pos.toString());
            auto land = db->getLandAt(pos, self.getDimensionId());
            if (PreCheckLandExistsAndPermission(land, self.getUuid().asString())) return;
            auto const& blockTypeName = self.getDimensionBlockSourceConst().getBlock(pos).getTypeName();
            CANCEL_AND_RETURN_IF(!land->getPermTable().allowAttackDragonEgg && blockTypeName == "minecraft:dragon_egg");
        });
    });

    RegisterListenerIf(Config::cfg.listeners.ArmorStandSwapItemBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::ArmorStandSwapItemBeforeEvent>(
            [db, logger](ila::mc::ArmorStandSwapItemBeforeEvent& ev) {
                Player& player = ev.player();
                logger->debug("[ArmorStandSwapItem]: executed");
                auto land = db->getLandAt(ev.self().getPosition(), player.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                    return;
                }
                if (land->getPermTable().useArmorStand) return;
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerDropItemBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerDropItemBeforeEvent>(
            [db, logger](ila::mc::PlayerDropItemBeforeEvent& ev) {
                Player& player = ev.self();
                logger->debug("[PlayerDropItem]: executed");
                auto land = db->getLandAt(player.getPosition(), player.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                    return;
                }
                if (land->getPermTable().allowDropItem) return;
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerOperatedItemFrameBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerOperatedItemFrameBeforeEvent>(
            [db, logger](ila::mc::PlayerOperatedItemFrameBeforeEvent& ev) {
                logger->debug("[PlayerUseItemFrame] pos: {}", ev.blockPos().toString());
                auto land = db->getLandAt(ev.blockPos(), ev.self().getDimensionId());
                if (PreCheckLandExistsAndPermission(land, ev.self().getUuid().asString())) return;
                if (land->getPermTable().useItemFrame) return;
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerEditSignBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerEditSignBeforeEvent>(
            [db, logger](ila::mc::PlayerEditSignBeforeEvent& ev) {
                auto& player = ev.self();
                auto& pos    = ev.pos();
                logger->debug("[PlayerEditSign] {} -> {}", player.getRealName(), pos.toString());
                auto land = db->getLandAt(pos, player.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                    return;
                }
                if (land && !land->getPermTable().editSign) {
                    ev.cancel();
                }
            }
        );
    });
}

} // namespace land
