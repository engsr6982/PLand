#include "pland/internal/interceptor/EventInterceptor.h"
#include "pland/internal/interceptor/InterceptorConfig.h"
#include "pland/internal/interceptor/helper/EventTrace.h"
#include "pland/internal/interceptor/helper/InterceptorHelper.h"

#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/event/player/PlayerDestroyBlockEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerPickUpItemEvent.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "ll/api/event/player/PlayerUseItemEvent.h"
#include <ll/api/event/EventBus.h>

#include "mc/deps/core/string/HashedString.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/BucketItem.h"
#include "mc/world/item/FishingRodItem.h"
#include "mc/world/item/FlintAndSteelItem.h"
#include "mc/world/item/HatchetItem.h"
#include "mc/world/item/HoeItem.h"
#include "mc/world/item/HorseArmorItem.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemTag.h"
#include "mc/world/item/ShovelItem.h"
#include "mc/world/level/block/BeaconBlock.h"
#include "mc/world/level/block/BedBlock.h"
#include "mc/world/level/block/BlastFurnaceBlock.h"
#include "mc/world/level/block/FurnaceBlock.h"
#include "mc/world/level/block/HangingSignBlock.h"
#include "mc/world/level/block/ShulkerBoxBlock.h"
#include "mc/world/level/block/SmokerBlock.h"
#include "pland/internal/interceptor/helper/VanillaItemTags.h"


namespace land::internal::interceptor {

void EventInterceptor::setupLLPlayerListeners() {
    auto& config   = InterceptorConfig::cfg.listeners;
    auto  registry = &PLand::getInstance().getLandRegistry();
    auto  bus      = &ll::event::EventBus::getInstance();

    registerListenerIf(config.PlayerDestroyBlockEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::PlayerDestroyBlockEvent>(
            [registry](ll::event::PlayerDestroyBlockEvent& ev) {
                TRACE_THIS_EVENT(ll::event::PlayerDestroyBlockEvent);

                auto& player = ev.self();
                auto& pos    = ev.pos();
                TRACE_LOG("player={}, pos={}", player.getRealName(), pos.toString());

                auto land = registry->getLandAt(pos, player.getDimensionId());
                if (!hasRolePermission<&RolePerms::allowDestroy>(land, player.getUuid())) {
                    ev.cancel();
                }
            }
        );
    });

    registerListenerIf(config.PlayerPlacingBlockEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::PlayerPlacingBlockEvent>(
            [registry](ll::event::PlayerPlacingBlockEvent& ev) {
                TRACE_THIS_EVENT(ll::event::PlayerPlacingBlockEvent);

                auto& player = ev.self();
                auto  pos    = ev.pos().relative(ev.face(), 1);
                TRACE_LOG("player={}, pos={}", player.getRealName(), pos.toString());

                auto land = registry->getLandAt(pos, player.getDimensionId());
                if (!hasRolePermission<&RolePerms::allowPlace>(land, player.getUuid())) {
                    ev.cancel();
                }
            }
        );
    });


    registerListenerIf(config.PlayerInteractBlockEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::PlayerInteractBlockEvent>(
            [registry](ll::event::PlayerInteractBlockEvent& ev) {
                TRACE_THIS_EVENT(ll::event::PlayerInteractBlockEvent);

                auto& player = ev.self();
                auto& uuid   = player.getUuid();
                auto& pos    = ev.blockPos();

                TRACE_LOG("player={}, pos={}, item={}", player.getRealName(), pos.toString(), ev.item().getTypeName());

                auto land = registry->getLandAt(pos, player.getDimensionId());
                if (hasPrivilege(land, uuid)) return;

                if (auto item = ev.item().getItem()) {
                    void** vftable = *reinterpret_cast<void** const*>(item);
                    if (vftable == BucketItem::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useBucket>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (vftable == HatchetItem::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useAxe>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (vftable == HoeItem::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useHoe>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (vftable == ShovelItem::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useShovel>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (item->hasTag(VanillaItemTags::Boats) || item->hasTag(VanillaItemTags::Boat)) {
                        if (!hasMemberOrGuestPermission<&RolePerms::placeBoat>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (item->hasTag(VanillaItemTags::Minecart)) {
                        if (!hasMemberOrGuestPermission<&RolePerms::placeMinecart>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (vftable == FlintAndSteelItem::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useFlintAndSteel>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    }
                    // fallback
                    if (auto pointer = InterceptorConfig::lookupDynamicRule(ev.item().getTypeName())) {
                        if (!_hasMemberOrGuestPermission(land, uuid, pointer)) {
                            ev.cancel();
                            return;
                        }
                    }
                }
                if (auto block = ev.block()) {
                    auto&  legacyBlock = block->getBlockType();
                    void** vftable     = *reinterpret_cast<void** const*>(&legacyBlock);
                    if (legacyBlock.isButtonBlock()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useButton>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (legacyBlock.isDoorBlock()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useDoor>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (legacyBlock.isFenceGateBlock()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useFenceGate>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (legacyBlock.isFenceBlock()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::allowInteractEntity>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (legacyBlock.mIsTrapdoor) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useTrapdoor>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (vftable == ShulkerBoxBlock::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useContainer>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (legacyBlock.isCraftingBlock()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useWorkstation>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (legacyBlock.isLeverBlock()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useLever>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (
                        vftable == BlastFurnaceBlock::$vftable() || vftable == FurnaceBlock::$vftable()
                        || vftable == SmokerBlock::$vftable()
                    ) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useFurnaces>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (vftable == BeaconBlock::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useBeacon>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    } else if (vftable == BedBlock::$vftable()) {
                        if (!hasMemberOrGuestPermission<&RolePerms::useBed>(land, uuid)) {
                            ev.cancel();
                            return;
                        }
                    }
                    // fallback
                    if (auto pointer = InterceptorConfig::lookupDynamicRule(block->getTypeName().data())) {
                        if (!_hasMemberOrGuestPermission(land, uuid, pointer)) {
                            ev.cancel();
                            return;
                        }
                    }
                }
            }
        );
    });

    registerListenerIf(config.PlayerAttackEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::PlayerAttackEvent>([registry](ll::event::PlayerAttackEvent& ev) {
            TRACE_THIS_EVENT(ll::event::PlayerAttackEvent);

            auto&    player = ev.self();
            auto&    target = ev.target();
            BlockPos pos    = target.getPosition();
            auto&    uuid   = player.getUuid();

            TRACE_LOG("player={}, target={}, pos={}", player.getRealName(), target.getTypeName(), pos.toString());

            auto land = registry->getLandAt(pos, player.getDimensionId());
            if (hasPrivilege(land, uuid)) return;

            if (target.getEntityTypeId() == ActorType::Player) {
                if (!hasMemberOrGuestPermission<&RolePerms::allowPvP>(land, uuid)) {
                    ev.cancel();
                    return;
                }
            }

            HashedString typeName{target.getTypeName()};

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
    registerListenerIf(config.PlayerPickUpItemEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::PlayerPickUpItemEvent>([registry](ll::event::PlayerPickUpItemEvent& ev) {
            TRACE_THIS_EVENT(ll::event::PlayerPickUpItemEvent);

            auto&    player = ev.self();
            auto&    item   = ev.itemActor();
            BlockPos pos    = item.getPosition();

            TRACE_LOG("player={}, item={}, pos={}", player.getRealName(), item.getTypeName(), pos.toString());

            auto land = registry->getLandAt(pos, player.getDimensionId());
            if (!hasRolePermission<&RolePerms::allowPlayerPickupItem>(land, player.getUuid())) {
                ev.cancel();
            }
        });
    });

    registerListenerIf(config.PlayerUseItemEvent, [bus, registry]() {
        return bus->emplaceListener<ll::event::PlayerUseItemEvent>([registry](ll::event::PlayerUseItemEvent& ev) {
            TRACE_THIS_EVENT(ll::event::PlayerUseItemEvent);

            auto& player    = ev.self();
            auto& itemStack = ev.item();
            auto  item      = itemStack.getItem();
            if (!item) {
                TRACE_LOG("item is nullptr");
                return;
            }

            TRACE_LOG("item={}, throwable={}", itemStack.getTypeName(), item->isThrowable());

            auto land = registry->getLandAt(player.getPosition(), player.getDimensionId());
            if (hasPrivilege(land, player.getUuid())) return;

            if (item->isThrowable()) {
                // 雪球、鸡蛋、末影珍珠、喷溅药水、滞留药水、附魔之瓶、冰弹、三叉戟
                if (!hasMemberOrGuestPermission<&RolePerms::allowUseThrowable>(land, player.getUuid())) {
                    ev.cancel();
                    return;
                }
            }

            auto typeName = HashedString{itemStack.getTypeName()};
            if (auto mptr = InterceptorConfig::lookupDynamicRule(typeName)) {
                if (!_hasMemberOrGuestPermission(land, player.getUuid(), mptr)) {
                    ev.cancel();
                }
            }
        });
    });
}


} // namespace land::internal::interceptor
