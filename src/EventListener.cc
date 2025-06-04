#include "pland/EventListener.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/world/SpawnMobEvent.h"


#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/shared_types/legacy/actor/ActorDamageCause.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Explosion.h"
#include "mc/world/level/block/BlockLegacy.h"
#include "mc/world/level/block/BlockProperty.h"

#include "mc/world/item/BucketItem.h"
#include "mc/world/item/FishingRodItem.h"
#include "mc/world/item/HatchetItem.h"
#include "mc/world/item/HoeItem.h"
#include "mc/world/item/HorseArmorItem.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemTag.h"
#include "mc/world/item/ShovelItem.h"
#include "mc/world/level/block/BlastFurnaceBlock.h"
#include "mc/world/level/block/FurnaceBlock.h"
#include "mc/world/level/block/HangingSignBlock.h"
#include "mc/world/level/block/ShulkerBoxBlock.h"
#include "mc/world/level/block/SignBlock.h"
#include "mc/world/level/block/SmokerBlock.h"
#include "mc/world/level/block/components/BlockComponentDirectData.h"
#include "mc/world/level/chunk/SubChunk.h"

#include "mc/world/phys/AABB.h"
#include "mc/world/phys/HitResult.h"


#include "mod/ModEntry.h"
#include "pland/Config.h"
#include "pland/DrawHandleManager.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandScheduler.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/math/LandAABB.h"
#include "pland/utils/McUtils.h"
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_set>
#include <vector>


#include "ll/api/event/entity/ActorHurtEvent.h"
#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/event/player/PlayerDestroyBlockEvent.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/event/player/PlayerPickUpItemEvent.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "ll/api/event/world/FireSpreadEvent.h"


#include "ila/event/minecraft/world/ExplosionEvent.h"
#include "ila/event/minecraft/world/PistonPushEvent.h"
#include "ila/event/minecraft/world/RedstoneUpdateEvent.h"
#include "ila/event/minecraft/world/SculkBlockGrowthEvent.h"
#include "ila/event/minecraft/world/WitherDestroyEvent.h"
#include "ila/event/minecraft/world/actor/ActorDestroyBlockEvent.h"
#include "ila/event/minecraft/world/actor/ActorRideEvent.h"
#include "ila/event/minecraft/world/actor/ActorTriggerPressurePlateEvent.h"
#include "ila/event/minecraft/world/actor/ArmorStandSwapItemEvent.h"
#include "ila/event/minecraft/world/actor/EndermanLeaveBlockEvent.h"
#include "ila/event/minecraft/world/actor/EndermanTakeBlockEvent.h"
#include "ila/event/minecraft/world/actor/MobHurtEffectEvent.h"
#include "ila/event/minecraft/world/actor/ProjectileCreateEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerAttackBlockEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerDropItemEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerEditSignEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerInteractEntityEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerOperatedItemFrameEvent.h"
#include "ila/event/minecraft/world/level/block/BlockFallEvent.h"
#include "ila/event/minecraft/world/level/block/DragonEggBlockTeleportEvent.h"
#include "ila/event/minecraft/world/level/block/FarmDecayEvent.h"
#include "ila/event/minecraft/world/level/block/LiquidFlowEvent.h"
#include "ila/event/minecraft/world/level/block/MossGrowthEvent.h"
#include "ila/event/minecraft/world/level/block/SculkCatalystAbsorbExperienceEvent.h"
#include "ila/event/minecraft/world/level/block/SculkSpreadEvent.h"


// Fix BlockProperty operator&
inline BlockProperty operator&(BlockProperty a, BlockProperty b) {
    return static_cast<BlockProperty>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

#define CANCEL_EVENT_AND_RETURN                                                                                        \
    ev.cancel();                                                                                                       \
    return;

#define CANCEL_AND_RETURN_IF(COND)                                                                                     \
    if (COND) {                                                                                                        \
        CANCEL_EVENT_AND_RETURN                                                                                        \
    }


namespace land {

inline bool PreCheckLandExistsAndPermission(LandData_sptr const& ptr, UUIDs const& uuid = "") {
    if (!ptr ||                                         // 无领地
        (PLand::getInstance().isOperator(uuid)) ||      // 管理员
        (ptr->getPermType(uuid) != LandPermType::Guest) // 主人/成员
    ) {
        return true;
    }
    return false;
}

static const std::unordered_map<std::string_view, bool LandPermTable::*> ItemSpecificPermissionMap = {
    {          "minecraft:skull",       &LandPermTable::allowPlace}, // 放置头颅
    {         "minecraft:banner",       &LandPermTable::allowPlace}, // 放置旗帜
    {   "minecraft:glow_ink_sac",       &LandPermTable::allowPlace}, // 荧光墨囊给告示牌上色
    {    "minecraft:end_crystal",       &LandPermTable::allowPlace}, // 放置末地水晶
    {      "minecraft:ender_eye",       &LandPermTable::allowPlace}, // 放置末影之眼
    {"minecraft:flint_and_steel", &LandPermTable::useFlintAndSteel}, // 使用打火石
    {      "minecraft:bone_meal",      &LandPermTable::useBoneMeal}, // 使用骨粉
    {    "minecraft:armor_stand",       &LandPermTable::allowPlace}  // 放置盔甲架
};

static const std::unordered_map<std::string_view, bool LandPermTable::*> BlockSpecificPermissionMap = {
    {                "minecraft:dragon_egg", &LandPermTable::allowAttackDragonEgg}, // 攻击龙蛋
    {                       "minecraft:bed",               &LandPermTable::useBed}, // 使用床
    {                     "minecraft:chest",       &LandPermTable::allowOpenChest}, // 打开箱子
    {             "minecraft:trapped_chest",       &LandPermTable::allowOpenChest}, // 打开陷阱箱
    {                  "minecraft:campfire",          &LandPermTable::useCampfire}, // 使用营火
    {             "minecraft:soul_campfire",          &LandPermTable::useCampfire}, // 使用灵魂营火
    {                 "minecraft:composter",         &LandPermTable::useComposter}, // 使用堆肥桶
    {                 "minecraft:noteblock",         &LandPermTable::useNoteBlock}, // 使用音符盒
    {                   "minecraft:jukebox",           &LandPermTable::useJukebox}, // 使用唱片机
    {                      "minecraft:bell",              &LandPermTable::useBell}, // 使用钟
    {"minecraft:daylight_detector_inverted",  &LandPermTable::useDaylightDetector}, // 使用阳光探测器 (反向)
    {         "minecraft:daylight_detector",  &LandPermTable::useDaylightDetector}, // 使用阳光探测器
    {                   "minecraft:lectern",           &LandPermTable::useLectern}, // 使用讲台
    {                  "minecraft:cauldron",          &LandPermTable::useCauldron}, // 使用炼药锅
    {            "minecraft:respawn_anchor",     &LandPermTable::useRespawnAnchor}, // 使用重生锚
    {                "minecraft:flower_pot",        &LandPermTable::editFlowerPot}, // 编辑花盆
    {          "minecraft:sweet_berry_bush",         &LandPermTable::allowDestroy}, // 收集甜浆果
};

static const std::unordered_map<std::string_view, bool LandPermTable::*> BlockFunctionalPermissionMap = {
    {   "minecraft:cartography_table",  &LandPermTable::useCartographyTable}, // 制图台
    {      "minecraft:smithing_table",     &LandPermTable::useSmithingTable}, // 锻造台
    {       "minecraft:brewing_stand",      &LandPermTable::useBrewingStand}, // 酿造台
    {               "minecraft:anvil",             &LandPermTable::useAnvil}, // 铁砧
    {          "minecraft:grindstone",        &LandPermTable::useGrindstone}, // 砂轮
    {    "minecraft:enchanting_table",   &LandPermTable::useEnchantingTable}, // 附魔台
    {              "minecraft:barrel",            &LandPermTable::useBarrel}, // 木桶
    {              "minecraft:beacon",            &LandPermTable::useBeacon}, // 信标
    {              "minecraft:hopper",            &LandPermTable::useHopper}, // 漏斗
    {             "minecraft:dropper",           &LandPermTable::useDropper}, // 投掷器
    {           "minecraft:dispenser",         &LandPermTable::useDispenser}, // 发射器
    {                "minecraft:loom",              &LandPermTable::useLoom}, // 织布机
    {   "minecraft:stonecutter_block",       &LandPermTable::useStonecutter}, // 切石机
    {             "minecraft:crafter",           &LandPermTable::useCrafter}, // 合成器
    {  "minecraft:chiseled_bookshelf", &LandPermTable::useChiseledBookshelf}, // 书架
    {                "minecraft:cake",              &LandPermTable::useCake}, //  蛋糕
    {"minecraft:unpowered_comparator",        &LandPermTable::useComparator}, //  比较器
    {  "minecraft:powered_comparator",        &LandPermTable::useComparator}, //  比较器
    {  "minecraft:unpowered_repeater",          &LandPermTable::useRepeater}, //  中继器
    {    "minecraft:powered_repeater",          &LandPermTable::useRepeater}, //  中继器
    {            "minecraft:bee_nest",           &LandPermTable::useBeeNest}, //  蜂巢
    {             "minecraft:beehive",           &LandPermTable::useBeeNest}, //
    {               "minecraft:vault",             &LandPermTable::useVault}  //  蜂箱
};


EventListener::EventListener() {
    auto* db     = &PLand::getInstance();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &mod::ModEntry::getInstance().getSelf().getLogger();


    mListenerPtrs = {
        bus->emplaceListener<ll::event::PlayerJoinEvent>([db, logger](ll::event::PlayerJoinEvent& ev) {
            if (ev.self().isSimulatedPlayer()) return;
            if (!db->hasPlayerSettings(ev.self().getUuid().asString())) {
                db->setPlayerSettings(ev.self().getUuid().asString(), PlayerSettings{}); // 新玩家
            }

            auto lands = db->getLands(ev.self().getXuid()); // xuid 查询
            if (!lands.empty()) {
                logger->info("Update land owner data from xuid to uuid for player {}", ev.self().getRealName());
                auto uuid = ev.self().getUuid().asString();
                for (auto& land : lands) {
                    if (land->mIsConvertedLand && land->mOwnerDataIsXUID) {
                        land->mLandOwner       = uuid; // xuid -> uuid
                        land->mOwnerDataIsXUID = false;
                    }
                }
            }
        }),
        bus->emplaceListener<ll::event::PlayerDisconnectEvent>([logger](ll::event::PlayerDisconnectEvent& ev) {
            auto& player = ev.self();
            if (player.isSimulatedPlayer()) return;
            logger->debug("Player {} disconnect, remove all cache");

            auto& uuid    = player.getUuid();
            auto  uuidStr = uuid.asString();

            GlobalPlayerLocaleCodeCached.erase(uuidStr);
            SelectorManager::getInstance().cancel(player);
            DrawHandleManager::getInstance().removeHandle(player);
            Require<LandScheduler>()->mLandidMap.erase(uuid);
            Require<LandScheduler>()->mDimidMap.erase(uuid);
        }),
    };

    // LL
    RegisterListenerIf(Config::cfg.listeners.ActorHurtEvent, [&]() {
        return bus->emplaceListener<ll::event::ActorHurtEvent>([db, logger](ll::event::ActorHurtEvent& ev) {
            auto&      hurtActor         = ev.self();
            auto&      damageSource      = ev.source();         // 伤害来源
            auto&      damageCause       = damageSource.mCause; // 伤害原因
            auto const damageActorType   = damageSource.getEntityType();
            bool const isPlayerDamage    = damageActorType == ActorType::Player; // 是否是玩家造成的伤害
            auto const hurtActorTypeName = hurtActor.getTypeName();
            logger->debug(
                "[ActorHurt] Mob: {}, ActorDamageCause: {}, ActorType: {} isPlayerDamage:{}",
                hurtActorTypeName,
                static_cast<int>(damageSource.mCause),
                static_cast<int>(damageSource.getEntityType()),
                isPlayerDamage
            );

            auto land = db->getLandAt(hurtActor.getPosition(), hurtActor.getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) return;

            if (isPlayerDamage) {
                if (auto souPlayer = hurtActor.getLevel().getPlayer(damageSource.getEntityUniqueID())) {
                    if ((damageCause == SharedTypes::Legacy::ActorDamageCause::EntityAttack
                         || damageCause == SharedTypes::Legacy::ActorDamageCause::Projectile)
                        && PreCheckLandExistsAndPermission(land, souPlayer->getUuid().asString())) {
                        return;
                    }
                }
            }

            auto const& tab = land->getLandPermTableConst();
            if (!tab.allowPlayerDamage && hurtActor.isPlayer()) {
                logger->debug(
                    "[ActorHurt] Cancel damage for player: {}, allowPlayerDamage is false",
                    hurtActor.getTypeName()
                );
                CANCEL_EVENT_AND_RETURN

            } else if (Config::cfg.mob.hostileMobTypeNames.contains(hurtActorTypeName)) {
                if (!tab.allowMonsterDamage) {
                    logger->debug(
                        "[ActorHurt] Cancel damage for monster: {}, allowMonsterDamage is false",
                        hurtActorTypeName
                    );
                    CANCEL_EVENT_AND_RETURN
                }

            } else if (Config::cfg.mob.specialMobTypeNames.contains(hurtActorTypeName)) {
                if (!tab.allowSpecialDamage) {
                    logger->debug(
                        "[ActorHurt] Cancel damage for special mob: {}, allowSpecialDamage is false",
                        hurtActorTypeName
                    );
                    CANCEL_EVENT_AND_RETURN
                }

            } else if (Config::cfg.mob.passiveMobTypeNames.contains(hurtActorTypeName)) {
                if (!tab.allowPassiveDamage) {
                    logger->debug(
                        "[ActorHurt] Cancel damage for passive mob: {}, allowPassiveDamage is false",
                        hurtActorTypeName
                    );
                    CANCEL_EVENT_AND_RETURN
                }

            } else if (Config::cfg.mob.customSpecialMobTypeNames.count(hurtActorTypeName)) {
                if (!tab.allowCustomSpecialDamage) {
                    logger->debug(
                        "[ActorHurt] Cancel damage for addon mob: {}, allowCustomSpecialDamage is false",
                        hurtActorTypeName
                    );
                    CANCEL_EVENT_AND_RETURN
                }
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerDestroyBlockEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerDestroyBlockEvent>(
            [db, logger](ll::event::PlayerDestroyBlockEvent& ev) {
                auto& player   = ev.self();
                auto& blockPos = ev.pos();

                logger->debug("[DestroyBlock] {}", blockPos.toString());

                auto land = db->getLandAt(blockPos, player.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                    return;
                }

                auto& tab = land->getLandPermTableConst();
                if (tab.allowDestroy) {
                    return;
                }

                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerPlacingBlockEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerPlacingBlockEvent>(
            [db, logger](ll::event::PlayerPlacingBlockEvent& ev) {
                auto&       player   = ev.self();
                auto const& blockPos = mc_utils::face2Pos(ev.pos(), ev.face()); // 计算实际放置位置

                logger->debug("[PlaceingBlock] {}", blockPos.toString());

                auto land = db->getLandAt(blockPos, player.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                    return;
                }

                auto& tab = land->getLandPermTableConst();
                if (tab.allowPlace) {
                    return;
                }

                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.ActorDestroyBlockEvent, [&]() {
        return bus->emplaceListener<ila::mc::ActorDestroyBlockEvent>([db, logger](ila::mc::ActorDestroyBlockEvent& ev) {
            auto& actor    = ev.self();
            auto& blockPos = ev.pos();

            logger->debug("[ActorDestroyBlock] Actor: {}, Pos: {}", actor.getTypeName(), blockPos.toString());

            auto land = db->getLandAt(blockPos, actor.getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) return; // land not found


            auto& tab = land->getLandPermTableConst();
            if (tab.allowActorDestroy) {
                return;
            }

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
                if (PreCheckLandExistsAndPermission(land)) return; // land not found


                auto& tab = land->getLandPermTableConst();
                if (tab.allowActorDestroy) {
                    return;
                }

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
                if (PreCheckLandExistsAndPermission(land)) return; // land not found


                auto& tab = land->getLandPermTableConst();
                if (tab.allowActorDestroy) {
                    return;
                }

                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerInteractBlockEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db, logger](
                                                                             ll::event::PlayerInteractBlockEvent& ev
                                                                         ) {
            auto& player    = ev.self();
            auto& pos       = ev.blockPos();
            auto& itemStack = ev.item();
            auto  block     = ev.block().has_value() ? &ev.block().get() : nullptr;

            // 从 ItemStack 获取 Item 对象
            const Item* actualItem = itemStack.getItem();

            auto const  itemTypeNameForMap = itemStack.getTypeName(); // 用于 itemSpecificPermissionMap 的键
            auto const& blockTypeName      = block ? block->getTypeName() : "";

            logger->debug(
                "[InteractBlock] Pos: {}, item: {} (Item*: {}), block: {}",
                pos.toString(),
                itemTypeNameForMap,
                (void*)actualItem,
                blockTypeName
            );

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                logger->debug(
                    "[InteractBlock] PreCheck returned true. Player: {}, Land: {}",
                    player.getUuid().asString(),
                    land ? land->getLandName() : "nullptr"
                );
                return;
            }

            auto const& tab        = land->getLandPermTableConst();
            bool        itemCancel = false;

            if (actualItem) {                                                      // 判空
                void** itemVftable = *reinterpret_cast<void** const*>(actualItem); // 获取物品的虚函数表
                auto&  itemTags    = *actualItem->mTags;
                for (auto const& tag : itemTags) {
                    logger->debug("Item Tag: {}", tag.getString());
                }

                if (itemVftable == BucketItem::$vftable()) {
                    if (!tab.useBucket) {
                        logger->debug("[InteractBlock] Item Cancel: BucketItem, useBucket is false");
                        itemCancel = true;
                    }
                } else if (itemVftable == HatchetItem::$vftable()) {
                    if (!tab.allowAxePeeled) {
                        logger->debug("[InteractBlock] Item Cancel: HatchetItem, allowAxePeeled is false");
                        itemCancel = true;
                    }
                } else if (itemVftable == HoeItem::$vftable()) {
                    if (!tab.useHoe) {
                        logger->debug("[InteractBlock] Item Cancel: HoeItem, useHoe is false");
                        itemCancel = true;
                    }
                } else if (itemVftable == ShovelItem::$vftable()) {
                    if (!tab.useShovel) {
                        logger->debug("[InteractBlock] Item Cancel: ShovelItem, useShovel is false");
                        itemCancel = true;
                    }
                } else if (actualItem->hasTag(HashedString("minecraft:boat"))) {
                    if (!tab.placeBoat) {
                        logger->debug("[InteractBlock] Item Cancel: minecraft:boat tag, placeBoat is false");
                        itemCancel = true;
                    }
                } else if (actualItem->hasTag(HashedString("minecraft:boats"))) {
                    if (!tab.placeBoat) {
                        logger->debug("[InteractBlock] Item Cancel: minecraft:boats tag, placeBoat is false");
                        itemCancel = true;
                    }
                } else if (actualItem->hasTag(HashedString("minecraft:is_minecart"))) {
                    if (!tab.placeMinecart) {
                        logger->debug("[InteractBlock] Item Cancel: minecraft:is_minecart tag, placeMinecart is false");
                        itemCancel = true;
                    }
                } else {
                    auto it = ItemSpecificPermissionMap.find(itemTypeNameForMap);
                    if (it != ItemSpecificPermissionMap.end()) {
                        if (!(tab.*(it->second))) {
                            logger->debug(
                                "[InteractBlock] Item Cancel: Specific item permission denied for {}",
                                itemTypeNameForMap
                            );
                            itemCancel = true;
                        } else {
                            logger->debug(
                                "[InteractBlock] Item Specific Permission Allowed: {} for {}",
                                itemTypeNameForMap,
                                tab.*(it->second)
                            );
                        }
                    } else {
                        logger->debug(
                            "[InteractBlock] Item: {} not found in itemSpecificPermissionMap",
                            itemTypeNameForMap
                        );
                    }
                }
            } else {
                auto it = ItemSpecificPermissionMap.find(itemTypeNameForMap);
                if (it != ItemSpecificPermissionMap.end()) {
                    if (!(tab.*(it->second))) {
                        logger->debug(
                            "[InteractBlock] Item Cancel (actualItem is null): Specific item permission denied for {}",
                            itemTypeNameForMap
                        );
                        itemCancel = true;
                    } else {
                        logger->debug(
                            "[InteractBlock] Item Specific Permission Allowed (actualItem is null): {} for {}",
                            itemTypeNameForMap,
                            tab.*(it->second)
                        );
                    }
                } else {
                    logger->debug(
                        "[InteractBlock] Item (actualItem is null): {} not found in itemSpecificPermissionMap",
                        itemTypeNameForMap
                    );
                }
            }
            CANCEL_AND_RETURN_IF(itemCancel);
            logger->debug("[InteractBlock] Item check passed.");

            if (block) { // 判空
                auto& blocktag = *block->mTags;
                for (auto const& btag : blocktag) {
                    logger->debug("Block Tag: {}", btag.mStr); // 方块的tag信息
                }

                auto const& legacyBlock = block->getLegacyBlock();
                bool        blockCancel = false;

                auto blockIter = BlockSpecificPermissionMap.find(blockTypeName);
                if (blockIter != BlockSpecificPermissionMap.end()) {
                    if (!(tab.*(blockIter->second))) {
                        logger->debug(
                            "[InteractBlock] Block Cancel: Specific block permission denied for {}",
                            blockTypeName
                        );
                        blockCancel = true;
                    } else {
                        logger->debug(
                            "[InteractBlock] Block Specific Permission Allowed: {} for {}",
                            blockTypeName,
                            tab.*(blockIter->second)
                        );
                    }
                } else {
                    logger->debug("[InteractBlock] Block: {} not found in blockSpecificPermissionMap", blockTypeName);
                }

                auto blockFuncIter = BlockFunctionalPermissionMap.find(blockTypeName);
                if (blockFuncIter != BlockFunctionalPermissionMap.end()) {
                    if (!(tab.*(blockFuncIter->second))) {
                        logger->debug(
                            "[InteractBlock] Block Cancel: Functional block permission denied for {}",
                            blockTypeName
                        );
                        blockCancel = true;
                    } else {
                        logger->debug(
                            "[InteractBlock] Block Functional Permission Allowed: {} for {}",
                            blockTypeName,
                            tab.*(blockFuncIter->second)
                        );
                    }
                } else {
                    logger->debug("[InteractBlock] Block: {} not found in blockFunctionalPermissionMap", blockTypeName);
                }

                CANCEL_AND_RETURN_IF(blockCancel);

                void** blockVftable = *reinterpret_cast<void** const*>(&legacyBlock);
                if (legacyBlock.isButtonBlock()) {
                    if (!tab.useButton) {
                        logger->debug("[InteractBlock] Block Cancel: Button, useButton is false");
                        blockCancel = true;
                    }
                } else if (legacyBlock.isDoorBlock()) {
                    if (!tab.useDoor) {
                        logger->debug("[InteractBlock] Block Cancel: Door, useDoor is false");
                        blockCancel = true;
                    }
                } else if (legacyBlock.isFenceGateBlock()) {
                    if (!tab.useFenceGate) {
                        logger->debug("[InteractBlock] Block Cancel: FenceGate, useFenceGate is false");
                        blockCancel = true;
                    }
                } else if (legacyBlock.isFenceBlock()) {
                    if (!tab.allowInteractEntity) {
                        // 往栅栏上挂拴绳
                        // TODO: 精确判定手上是否有拴住的实体
                        logger->debug("[InteractBlock] Block Cancel: Fence, allowInteractEntity is false");
                        blockCancel = true;
                    }
                } else if (legacyBlock.mIsTrapdoor) {
                    if (!tab.useTrapdoor) {
                        logger->debug("[InteractBlock] Block Cancel: Trapdoor, useTrapdoor is false");
                        blockCancel = true;
                    }
                } else if (blockVftable == SignBlock::$vftable() || blockVftable == HangingSignBlock::$vftable()) {
                    if (!tab.editSign) {
                        logger->debug("[InteractBlock] Block Cancel: Sign, editSign is false");
                        blockCancel = true;
                    }
                } else if (blockVftable == ShulkerBoxBlock::$vftable()) {
                    if (!tab.useShulkerBox) {
                        logger->debug("[InteractBlock] Block Cancel: ShulkerBox, useShulkerBox is false");
                        blockCancel = true;
                    }
                } else if (legacyBlock.isCraftingBlock()) {
                    if (!tab.useCraftingTable) {
                        logger->debug("[InteractBlock] Block Cancel: CraftingTable, useCraftingTable is false");
                        blockCancel = true;
                    }
                } else if (legacyBlock.isLeverBlock()) {
                    if (!tab.useLever) {
                        logger->debug("[InteractBlock] Block Cancel: Lever, useLever is false");
                        blockCancel = true;
                    }
                } else if (blockVftable == BlastFurnaceBlock::$vftable()) {
                    if (!tab.useBlastFurnace) {
                        logger->debug("[InteractBlock] Block Cancel: BlastFurnace, useBlastFurnace is false");
                        blockCancel = true;
                    }
                } else if (blockVftable == FurnaceBlock::$vftable()) {
                    if (!tab.useFurnace) {
                        logger->debug("[InteractBlock] Block Cancel: Furnace, useFurnace is false");
                        blockCancel = true;
                    }
                } else if (blockVftable == SmokerBlock::$vftable()) {
                    if (!tab.useSmoker) {
                        logger->debug("[InteractBlock] Block Cancel: Smoker, useSmoker is false");
                        blockCancel = true;
                    }
                }
                CANCEL_AND_RETURN_IF(blockCancel);
                logger->debug("[InteractBlock] Block check passed.");
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerInteractEntityBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerInteractEntityBeforeEvent>(
            [db, logger](ila::mc::PlayerInteractEntityBeforeEvent& ev) {
                logger->debug("[交互实体] name: {}", ev.self().getRealName());
                auto& entity = ev.target();
                auto  land   = db->getLandAt(entity.getPosition(), ev.self().getDimensionId());
                if (PreCheckLandExistsAndPermission(land, ev.self().getUuid().asString())) return;

                if (land->getLandPermTableConst().allowInteractEntity) return;

                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.FireSpreadEvent, [&]() {
        return bus->emplaceListener<ll::event::FireSpreadEvent>([db](ll::event::FireSpreadEvent& ev) {
            auto& pos = ev.pos();

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) {
                return;
            }

            if (land->getLandPermTableConst().allowFireSpread) {
                return;
            }

            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerAttackEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerAttackEvent>([db, logger](ll::event::PlayerAttackEvent& ev) {
            auto& player = ev.self();
            auto& mob    = ev.target();
            auto& pos    = mob.getPosition();

            logger->debug("[AttackEntity] Entity: {}, Pos: {}", mob.getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                return;
            }

            auto const& et  = mob.getTypeName();
            auto const& tab = land->getLandPermTableConst();
            if ((et == "minecraft:armor_stand" && !tab.allowDestroyArmorStand) ||    // 盔甲架
                (et == "minecraft:ender_crystal" && !tab.allowAttackEnderCrystal) || // 末影水晶
                (et == "minecraft:painting" && !tab.allowAttackPainting) ||          // 画
                (mob.hasFamily("boat") && !tab.allowAttackBoat) ||                   // 船
                (mob.hasFamily("minecart") && !tab.allowAttackMinecart)              // 矿车
            ) {
                ev.cancel();
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerPickUpItemEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerPickUpItemEvent>([db,
                                                                       logger](ll::event::PlayerPickUpItemEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.itemActor().getPosition();

            logger->debug("[PickUpItem] Item: {}, Pos: {}", ev.itemActor().getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                return;
            }

            if (land->getLandPermTableConst().allowPickupItem) return;

            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.SpawnedMobEvent, [&]() {
        return bus->emplaceListener<ll::event::SpawnedMobEvent>([db, logger](ll::event::SpawnedMobEvent& ev) {
            auto mob = ev.mob();
            if (!mob.has_value()) {
                return;
            }

            auto& pos = mob->getPosition();

            logger->debug("[SpawnedMob] {}", pos.toString());

            auto land = db->getLandAt(pos, mob->getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) {
                return;
            }

            auto const& tab       = land->getLandPermTableConst();
            bool        isMonster = mob->hasCategory(::ActorCategory::Monster) || mob->hasFamily("monster");

            if (isMonster) {
                if (!tab.allowMonsterSpawn) {
                    mob->despawn();
                }
            } else {
                if (!tab.allowAnimalSpawn) {
                    mob->despawn();
                }
            }
        });
    });


    // ila
    RegisterListenerIf(Config::cfg.listeners.PlayerAttackBlockBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerAttackBlockBeforeEvent>(
            [db, logger](ila::mc::PlayerAttackBlockBeforeEvent& ev) {
                auto& self = ev.self();
                auto& pos  = ev.pos();

                logger->debug("[AttackBlock] {}", pos.toString());

                auto land = db->getLandAt(pos, self.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, self.getUuid().asString())) return; // land not found

                auto const& blockTypeName = self.getDimensionBlockSourceConst().getBlock(pos).getTypeName();
                CANCEL_AND_RETURN_IF(
                    !land->getLandPermTableConst().allowAttackDragonEgg && blockTypeName == "minecraft:dragon_egg"
                );
            }
        );
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

                if (land->getLandPermTableConst().useArmorStand) return;

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

                if (land->getLandPermTableConst().allowDropItem) return;

                ev.cancel();
            }
        );
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

            if (!passenger.isPlayer()) {
                return; // 忽略非玩家骑乘事件
            }

            auto const& typeName = ev.target().getTypeName();
            auto        land     = db->getLandAt(target.getPosition(), target.getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) return; // land not found
            // 特殊处理：
            if (land) {
                auto& tab = land->getLandPermTableConst();
                if (typeName == "minecraft:minecart" || typeName == "minecraft:boat") {
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

    RegisterListenerIf(Config::cfg.listeners.ExplosionBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::ExplosionBeforeEvent>([db, logger](ila::mc::ExplosionBeforeEvent& ev) {
            logger->debug("[Explode] Pos: {}", ev.explosion().mPos->toString());

            auto lands = db->getLandAt(
                BlockPos{ev.explosion().mPos},
                (int)(ev.explosion().mRadius + 1.0),
                ev.blockSource().getDimensionId()
            );
            for (auto& p : lands) {
                if (!p->getLandPermTableConst().allowExplode) {
                    ev.cancel();
                    break;
                }
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.FarmDecayBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::FarmDecayBeforeEvent>([db, logger](ila::mc::FarmDecayBeforeEvent& ev) {
            logger->debug("[FarmDecay] Pos: {}", ev.pos().toString());

            auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
            if (PreCheckLandExistsAndPermission(land)) return; // land not found
            if (land) {
                if (land->getLandPermTableConst().allowFarmDecay) return;
            }

            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.MobHurtEffectBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::MobHurtEffectBeforeEvent>([db,
                                                                        logger](ila::mc::MobHurtEffectBeforeEvent& ev) {
            auto&      hurtActor         = ev.self();
            auto const hurtActorTypeName = hurtActor.getTypeName();
            logger->debug("[MobHurtEffect] mob: {}", hurtActorTypeName);
            bool const hurtActorIsPlayer = hurtActor.isPlayer();

            auto land = db->getLandAt(hurtActor.getPosition(), hurtActor.getDimensionId());
            if (!land) return;

            // 放行来自有权限的玩家伤害
            if (auto source = ev.source(); source && source->isPlayer()) {
                auto& player = static_cast<Player&>(source.value());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) return; // land not found
            }

            auto const& tab = land->getLandPermTable();
            if (hurtActorIsPlayer) {
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

    RegisterListenerIf(Config::cfg.listeners.PistonPushBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PistonPushBeforeEvent>([db, logger](ila::mc::PistonPushBeforeEvent& ev) {
            auto const& piston = ev.pistonPos();
            auto const& push   = ev.pushPos();

            logger->debug("[PistonTryPush] piston: {}, push: {}", piston.toString(), push.toString());

            auto const dimid = ev.blockSource().getDimensionId();

            auto pistonLand = db->getLandAt(piston, dimid);
            auto pushLand   = db->getLandAt(push, dimid);
            if ((!pistonLand &&                                               // 活塞所在位置没有领地
                 pushLand &&                                                  // 被推动的位置有领地
                 !pushLand->getLandPermTableConst().allowPistonPushOnBoundary // 被推动的位置领地不允许活塞在边界推动
                 && (pushLand->getLandPos().isOnOuterBoundary(piston) || pushLand->getLandPos().isOnInnerBoundary(push))
                ) // 被推动的位置领地在边界上 (活塞在边界或边界外，被推的方块在边界内)
                || (pistonLand && pushLand && pistonLand != pushLand
                ) // 活塞和被推动的位置不在同一个领地 (例如：父子领地/无间距领地)
            ) {
                ev.cancel();
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerOperatedItemFrameBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::PlayerOperatedItemFrameBeforeEvent>(
            [db, logger](ila::mc::PlayerOperatedItemFrameBeforeEvent& ev) {
                logger->debug("[PlayerUseItemFrame] pos: {}", ev.blockPos().toString());

                auto land = db->getLandAt(ev.blockPos(), ev.self().getDimensionId());
                if (PreCheckLandExistsAndPermission(land, ev.self().getUuid().asString())) return;

                if (land->getLandPermTableConst().useItemFrame) return;

                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.ActorTriggerPressurePlateBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::ActorTriggerPressurePlateBeforeEvent>(
            [db, logger](ila::mc::ActorTriggerPressurePlateBeforeEvent& ev) {
                logger->debug("[PressurePlateTrigger] pos: {}", ev.pos().toString());

                auto land = db->getLandAt(ev.pos(), ev.self().getDimensionId());
                if (land && land->getLandPermTableConst().usePressurePlate) return;
                if (PreCheckLandExistsAndPermission(land)) return; // land not found

                auto& entity = ev.self();
                if (entity.isPlayer()) {
                    auto pl = entity.getWeakEntity().tryUnwrap<Player>();
                    if (pl.has_value()) {
                        if (PreCheckLandExistsAndPermission(land, pl->getUuid().asString())) return;
                    }
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
                if (!mob) {
                    return;
                }
                auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
                if (PreCheckLandExistsAndPermission(land)) return; // land not found

                if (self.getOwnerEntityType() == ActorType::Player) {
                    // 由玩家所创建的实体
                    if (mob->isPlayer()) {
                        auto pl = mob->getWeakEntity().tryUnwrap<Player>();
                        if (pl.has_value() && PreCheckLandExistsAndPermission(land, pl->getUuid().asString())) return;
                    }
                }

                if (land) {
                    auto const& tab = land->getLandPermTableConst();
                    if (mob->isPlayer()) {
                        // 钓鱼钩单独判断，其他抛射物统一判断
                        if (type == "minecraft:fishing_hook") {
                            CANCEL_AND_RETURN_IF(!tab.useFishingHook);
                        } else {
                            CANCEL_AND_RETURN_IF(!tab.allowProjectileCreate);
                        }
                    }
                    return;
                }
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.RedstoneUpdateBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::RedstoneUpdateBeforeEvent>(
            [db, logger](ila::mc::RedstoneUpdateBeforeEvent& ev) {
                // logger->debug("[RedstoneUpdate] Pos: {}", ev.Pos().toString());

                auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
                if (PreCheckLandExistsAndPermission(land)) return; // land not found
                if (land) {
                    if (land->getLandPermTableConst().allowRedstoneUpdate) return;
                }

                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.BlockFallBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::BlockFallBeforeEvent>([db, logger](ila::mc::BlockFallBeforeEvent& ev) {
            logger->debug("[BlockFall] Pos: {}", ev.pos().toString());

            auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
            if (land) {
                auto const& tab = land->getLandPermTableConst();
                // 不允许方块掉落
                CANCEL_AND_RETURN_IF(!tab.allowBlockFall);

                // 位于领地之外（上方）的方块下落，且领地不允许方块下落，则拦截
                if (land->getLandPos().isAboveLand(ev.pos()) && !tab.allowBlockFall) {
                    logger->debug("[BlockFall] Block fall above land, cancelled for land: {}", land->getLandName());
                    CANCEL_EVENT_AND_RETURN
                }
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.WitherDestroyBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::WitherDestroyBeforeEvent>([db,
                                                                        logger](ila::mc::WitherDestroyBeforeEvent& ev) {
            logger->debug("[WitherDestroyBlock] executed");
            auto& aabb = ev.box();

            auto lands = db->getLandAt(aabb.min, aabb.max, ev.blockSource().getDimensionId());
            for (auto const& p : lands) {
                if (!p->getLandPermTableConst().allowWitherDestroy) {
                    ev.cancel();
                    break;
                }
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.MossGrowthBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::MossGrowthBeforeEvent>([db, logger](ila::mc::MossGrowthBeforeEvent& ev) {
            // logger->debug("[MossSpread] {}", ev.Pos().toString());

            auto const& pos = ev.pos();

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (!land || land->getLandPermTableConst().useBoneMeal) {
                return;
            }

            auto lds = db->getLandAt(pos - 9, pos + 9, ev.blockSource().getDimensionId());
            for (auto const& p : lds) {
                if (p->getLandPermTableConst().useBoneMeal) {
                    return;
                }
            }

            ev.cancel();
        });
    });

    RegisterListenerIf(Config::cfg.listeners.LiquidTryFlowBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::LiquidFlowBeforeEvent>([db, logger](ila::mc::LiquidFlowBeforeEvent& ev) {
            auto& sou = ev.flowFromPos();
            auto& to  = ev.pos();
            // logger->debug("[LiquidFlow] {} -> {}", sou.toString(), to.toString());

            auto landSou = db->getLandAt(sou, ev.blockSource().getDimensionId());
            auto landTo  = db->getLandAt(to, ev.blockSource().getDimensionId());

            // 源头在领地外
            if (landTo) { // 判空
                if (!landTo->getLandPermTableConst().allowLiquidFlow && landTo->getLandPos().isOnOuterBoundary(sou)
                    && landTo->getLandPos().isOnInnerBoundary(to)) {
                    logger->debug("[LiquidFlow] 液体流动成功拦截 {}", landTo->getLandName());
                    ev.cancel();
                }
                logger->debug("[LiquidFlow] 液体流动: {}", landTo->getLandName());
            }
            // logger->debug("[LiquidFlow] Land:null");
        });
    });

    RegisterListenerIf(Config::cfg.listeners.DragonEggBlockTeleportBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::DragonEggBlockTeleportBeforeEvent>(
            [db, logger](ila::mc::DragonEggBlockTeleportBeforeEvent& ev) {
                auto& pos = ev.pos();
                logger->debug("[DragonEggBlockTeleport] {}", pos.toString());

                auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
                if (land) {
                    if (!land->getLandPermTableConst().allowAttackDragonEgg) {
                        ev.cancel();
                    }
                }
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.SculkBlockGrowthBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::SculkBlockGrowthBeforeEvent>(
            [db, logger](ila::mc::SculkBlockGrowthBeforeEvent& ev) {
                auto& pos = ev.pos();
                logger->debug("[SculkBlockGrowth] {}", pos.toString());

                auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
                if (land) {
                    if (!land->getLandPermTableConst().allowSculkBlockGrowth) {
                        ev.cancel();
                    }
                }
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.SculkSpreadBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::SculkSpreadBeforeEvent>([db, logger](ila::mc::SculkSpreadBeforeEvent& ev) {
            // logger->debug("[SculkSpread] {} -> {}", ev.SelfPos().toString(), ev.TargetPos().toString());

            auto sou = db->getLandAt(ev.selfPos(), ev.blockSource().getDimensionId());
            auto tar = db->getLandAt(ev.targetPos(), ev.blockSource().getDimensionId());

            // if (!sou && !tar) return; // 领地外蔓延
            // if (sou && tar) return;   // 领地内蔓延
            // if (sou && !tar) return;  // 领地内蔓延到外
            if (!sou && tar) {
                ev.cancel(); // 外蔓延到领地内
            }
        });
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

                if (land && !land->getLandPermTableConst().editSign) {
                    ev.cancel();
                }
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.SculkCatalystAbsorbExperienceBeforeEvent, [&]() {
        return bus->emplaceListener<ila::mc::SculkCatalystAbsorbExperienceBeforeEvent>(
            [db, logger](ila::mc::SculkCatalystAbsorbExperienceBeforeEvent& ev) {
                // auto& actor  = ev.DiedActor();
                auto& actor  = ev.actor();
                auto& region = actor.getDimensionBlockSource();
                auto  pos    = actor.getBlockPosCurrentlyStandingOn(&actor);
                logger->debug("[SculkCatalystAbsorbExperience] Pos: {}", pos.toString());
                // 领地内蔓延 && 半径内没有别的领地 => 放行
                // 领地外蔓延 && 半径内有别的领地   => 放行
                auto cur = db->getLandAt(pos, region.getDimensionId());
                auto lds = db->getLandAt(pos - 9, pos + 9, region.getDimensionId());
                if (cur && lds.size() == 1) return;
                if (!cur && lds.empty()) return;
                ev.cancel();
            }
        );
    });
}
#undef CANCEL_AND_RETURN_IF

EventListener::~EventListener() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& ptr : mListenerPtrs) {
        bus.removeListener(ptr);
    }
}


void EventListener::RegisterListenerIf(bool need, std::function<ll::event::ListenerPtr()> const& factory) {
    if (need) {
        auto listenerPtr = factory();
        mListenerPtrs.push_back(std::move(listenerPtr));
    }
}


} // namespace land
