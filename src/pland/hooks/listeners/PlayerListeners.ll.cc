
#include "pland/hooks/EventListener.h"
#include "pland/hooks/listeners/ListenerHelper.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/event/player/PlayerDestroyBlockEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerPickUpItemEvent.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"

#include "mc/deps/core/string/HashedString.h"
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

#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"
#include "pland/utils/McUtils.h"
#include <string_view>
#include <unordered_map>


namespace land {

// These maps are used by PlayerInteractBlockEvent, so they stay in this file.
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


void EventListener::registerLLPlayerListeners() {
    auto* db     = PLand::getInstance().getLandRegistry();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &land::PLand::getInstance().getSelf().getLogger();

    RegisterListenerIf(Config::cfg.listeners.PlayerDestroyBlockEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerDestroyBlockEvent>(
            [db, logger](ll::event::PlayerDestroyBlockEvent& ev) {
                auto& player   = ev.self();
                auto& blockPos = ev.pos();
                logger->debug(
                    "[DestroyBlock] Player: {}({}), Pos: {}",
                    player.getRealName(),
                    player.getUuid().asString(),
                    blockPos.toString()
                );
                auto land = db->getLandAt(blockPos, player.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                    logger->debug("[DestroyBlock] No land or player has permission. Allowed.");
                    return;
                }
                auto& tab = land->getPermTable();
                if (tab.allowDestroy) {
                    logger->debug("[DestroyBlock] Permission 'allowDestroy' is true. Allowed.");
                    return;
                }
                logger->debug("[DestroyBlock] Permission 'allowDestroy' is false. Cancelled.");
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerPlacingBlockEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerPlacingBlockEvent>(
            [db, logger](ll::event::PlayerPlacingBlockEvent& ev) {
                auto&       player   = ev.self();
                auto const& blockPos = mc_utils::face2Pos(ev.pos(), ev.face());
                logger->debug(
                    "[PlaceBlock] Player: {}({}), Pos: {}",
                    player.getRealName(),
                    player.getUuid().asString(),
                    blockPos.toString()
                );
                auto land = db->getLandAt(blockPos, player.getDimensionId());
                if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                    logger->debug("[PlaceBlock] No land or player has permission. Allowed.");
                    return;
                }
                auto& tab = land->getPermTable();
                if (tab.allowPlace) {
                    logger->debug("[PlaceBlock] Permission 'allowPlace' is true. Allowed.");
                    return;
                }
                logger->debug("[PlaceBlock] Permission 'allowPlace' is false. Cancelled.");
                ev.cancel();
            }
        );
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerInteractBlockEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db, logger](
                                                                             ll::event::PlayerInteractBlockEvent& ev
                                                                         ) {
            auto&       player             = ev.self();
            auto&       pos                = ev.blockPos();
            auto&       itemStack          = ev.item();
            auto        block              = ev.block().has_value() ? &ev.block().get() : nullptr;
            const Item* actualItem         = itemStack.getItem();
            auto const  itemTypeNameForMap = itemStack.getTypeName();
            auto const& blockTypeName      = block ? block->getTypeName() : "";
            logger->debug(
                "[InteractBlock] Player: {}({}), Pos: {}, Item: {}, Block: {}",
                player.getRealName(),
                player.getUuid().asString(),
                pos.toString(),
                itemTypeNameForMap,
                blockTypeName
            );
            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                logger->debug("[InteractBlock] No land or player has permission. Allowed.");
                return;
            }
            auto const& tab        = land->getPermTable();
            bool        itemCancel = false;
            if (actualItem) {
                void** itemVftable = *reinterpret_cast<void** const*>(actualItem);
                if (itemVftable == BucketItem::$vftable()) {
                    if (!tab.useBucket) {
                        logger->debug("[InteractBlock] Item check: BucketItem, 'useBucket' is false. Cancelled.");
                        itemCancel = true;
                    }
                } else if (itemVftable == HatchetItem::$vftable()) {
                    if (!tab.allowAxePeeled) {
                        logger->debug("[InteractBlock] Item check: HatchetItem, 'allowAxePeeled' is false. Cancelled.");
                        itemCancel = true;
                    }
                } else if (itemVftable == HoeItem::$vftable()) {
                    if (!tab.useHoe) {
                        logger->debug("[InteractBlock] Item check: HoeItem, 'useHoe' is false. Cancelled.");
                        itemCancel = true;
                    }
                } else if (itemVftable == ShovelItem::$vftable()) {
                    if (!tab.useShovel) {
                        logger->debug("[InteractBlock] Item check: ShovelItem, 'useShovel' is false. Cancelled.");
                        itemCancel = true;
                    }
                } else if (actualItem->hasTag(HashedString("minecraft:boat"))
                           || actualItem->hasTag(HashedString("minecraft:boats"))) {
                    if (!tab.placeBoat) {
                        logger->debug("[InteractBlock] Item check: Boat, 'placeBoat' is false. Cancelled.");
                        itemCancel = true;
                    }
                } else if (actualItem->hasTag(HashedString("minecraft:is_minecart"))) {
                    if (!tab.placeMinecart) {
                        logger->debug("[InteractBlock] Item check: Minecart, 'placeMinecart' is false. Cancelled.");
                        itemCancel = true;
                    }
                } else {
                    auto it = ItemSpecificPermissionMap.find(itemTypeNameForMap);
                    if (it != ItemSpecificPermissionMap.end() && !(tab.*(it->second))) {
                        logger->debug(
                            "[InteractBlock] Item check: '{}', specific permission is false. Cancelled.",
                            itemTypeNameForMap
                        );
                        itemCancel = true;
                    }
                }
            } else {
                auto it = ItemSpecificPermissionMap.find(itemTypeNameForMap);
                if (it != ItemSpecificPermissionMap.end() && !(tab.*(it->second))) {
                    logger->debug(
                        "[InteractBlock] Item check (no item*): '{}', specific permission is false. Cancelled.",
                        itemTypeNameForMap
                    );
                    itemCancel = true;
                }
            }
            CANCEL_AND_RETURN_IF(itemCancel);
            logger->debug("[InteractBlock] Item checks passed.");

            if (block) {
                auto const& legacyBlock = block->getLegacyBlock();
                bool        blockCancel = false;

                auto log_cancel = [&](const std::string& perm_name) {
                    logger->debug(
                        "[InteractBlock] Block check: '{}', permission '{}' is false. Cancelled.",
                        blockTypeName,
                        perm_name
                    );
                    blockCancel = true;
                };

                auto blockIter = BlockSpecificPermissionMap.find(blockTypeName);
                if (blockIter != BlockSpecificPermissionMap.end() && !(tab.*(blockIter->second))) {
                    log_cancel("BlockSpecificPermission");
                    CANCEL_AND_RETURN_IF(blockCancel);
                }
                auto blockFuncIter = BlockFunctionalPermissionMap.find(blockTypeName);
                if (blockFuncIter != BlockFunctionalPermissionMap.end() && !(tab.*(blockFuncIter->second))) {
                    log_cancel("BlockFunctionalPermission");
                    CANCEL_AND_RETURN_IF(blockCancel);
                }

                void** blockVftable = *reinterpret_cast<void** const*>(&legacyBlock);
                if (legacyBlock.isButtonBlock()) {
                    if (!tab.useButton) log_cancel("useButton");
                } else if (legacyBlock.isDoorBlock()) {
                    if (!tab.useDoor) log_cancel("useDoor");
                } else if (legacyBlock.isFenceGateBlock()) {
                    if (!tab.useFenceGate) log_cancel("useFenceGate");
                } else if (legacyBlock.isFenceBlock()) {
                    if (!tab.allowInteractEntity) log_cancel("allowInteractEntity");
                } else if (legacyBlock.mIsTrapdoor) {
                    if (!tab.useTrapdoor) log_cancel("useTrapdoor");
                } else if (blockVftable == SignBlock::$vftable() || blockVftable == HangingSignBlock::$vftable()) {
                    if (!tab.editSign) log_cancel("editSign");
                } else if (blockVftable == ShulkerBoxBlock::$vftable()) {
                    if (!tab.useShulkerBox) log_cancel("useShulkerBox");
                } else if (legacyBlock.isCraftingBlock()) {
                    if (!tab.useCraftingTable) log_cancel("useCraftingTable");
                } else if (legacyBlock.isLeverBlock()) {
                    if (!tab.useLever) log_cancel("useLever");
                } else if (blockVftable == BlastFurnaceBlock::$vftable()) {
                    if (!tab.useBlastFurnace) log_cancel("useBlastFurnace");
                } else if (blockVftable == FurnaceBlock::$vftable()) {
                    if (!tab.useFurnace) log_cancel("useFurnace");
                } else if (blockVftable == SmokerBlock::$vftable()) {
                    if (!tab.useSmoker) log_cancel("useSmoker");
                }
                CANCEL_AND_RETURN_IF(blockCancel);
                logger->debug("[InteractBlock] Block checks passed.");
            }
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerAttackEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerAttackEvent>([db, logger](ll::event::PlayerAttackEvent& ev) {
            auto& player = ev.self();
            auto& mob    = ev.target();
            auto& pos    = mob.getPosition();
            logger->debug(
                "[AttackEntity] Player: {}({}), Target: {}, Pos: {}",
                player.getRealName(),
                player.getUuid().asString(),
                mob.getTypeName(),
                pos.toString()
            );
            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                logger->debug("[AttackEntity] No land or player has permission. Allowed.");
                return;
            }
            auto const& mobTypeName = mob.getTypeName();
            auto const& tab         = land->getPermTable();

            auto check_perm = [&](bool has_perm, const std::string& perm_name) {
                if (!has_perm) {
                    logger->debug(
                        "[AttackEntity] Permission '{}' is false for mob '{}'. Cancelled.",
                        perm_name,
                        mobTypeName
                    );
                    ev.cancel();
                    return true;
                }
                return false;
            };

            if (Config::cfg.mob.hostileMobTypeNames.contains(mobTypeName)) {
                if (check_perm(tab.allowMonsterDamage, "allowMonsterDamage")) return;
            } else if (Config::cfg.mob.specialMobTypeNames.contains(mobTypeName)) {
                if (check_perm(tab.allowSpecialDamage, "allowSpecialDamage")) return;
            } else if (mobTypeName == "minecraft:player") {
                if (check_perm(tab.allowPlayerDamage, "allowPlayerDamage")) return;
            } else if (Config::cfg.mob.passiveMobTypeNames.contains(mobTypeName)) {
                if (check_perm(tab.allowPassiveDamage, "allowPassiveDamage")) return;
            } else if (Config::cfg.mob.customSpecialMobTypeNames.count(mobTypeName)) {
                if (check_perm(tab.allowCustomSpecialDamage, "allowCustomSpecialDamage")) return;
            }
            logger->debug("[AttackEntity] All permission checks passed. Allowed.");
        });
    });

    RegisterListenerIf(Config::cfg.listeners.PlayerPickUpItemEvent, [&]() {
        return bus->emplaceListener<ll::event::PlayerPickUpItemEvent>([db,
                                                                       logger](ll::event::PlayerPickUpItemEvent& ev) {
            auto& player = ev.self();
            auto& item   = ev.itemActor();
            auto& pos    = item.getPosition();
            logger->debug(
                "[PickUpItem] Player: {}({}), Item: {}, Pos: {}",
                player.getRealName(),
                player.getUuid().asString(),
                item.getTypeName(),
                pos.toString()
            );
            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheckLandExistsAndPermission(land, player.getUuid().asString())) {
                logger->debug("[PickUpItem] No land or player has permission. Allowed.");
                return;
            }
            if (land->getPermTable().allowPickupItem) {
                logger->debug("[PickUpItem] Permission 'allowPickupItem' is true. Allowed.");
                return;
            }
            logger->debug("[PickUpItem] Permission 'allowPickupItem' is false. Cancelled.");
            ev.cancel();
        });
    });
}

} // namespace land
