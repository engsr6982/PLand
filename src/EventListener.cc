#include "pland/EventListener.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/event/player/PlayerDestroyBlockEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/event/player/PlayerPickUpItemEvent.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "ll/api/event/player/PlayerUseItemEvent.h"
#include "ll/api/event/world/FireSpreadEvent.h"
#include "ll/api/event/world/SpawnMobEvent.h"
#include "mc/network/packet/UpdateBlockPacket.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/level/material/Material.h"
#include "mc/world/phys/HitResult.h"
#include "mod/MyMod.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include "pland/utils/MC.h"
#include <functional>
#include <unordered_map>

#include "ArmorStandSwapItemEvent.h"
#include "PlayerAttackBlockEvent.h"
#include "PlayerDropItemEvent.h"


ll::event::ListenerPtr mPlayerDestroyBlockEvent;  // 玩家尝试破坏方块
ll::event::ListenerPtr mPlayerPlaceingBlockEvent; // 玩家尝试放置方块
ll::event::ListenerPtr mPlayerUseItemOnEvent;     // 玩家对方块使用物品（点击右键）
ll::event::ListenerPtr mFireSpreadEvent;          // 火焰蔓延
ll::event::ListenerPtr mPlayerAttackEntityEvent;  // 玩家攻击实体
ll::event::ListenerPtr mPlayerPickUpItemEvent;    // 玩家捡起物品
ll::event::ListenerPtr mPlayerInteractBlockEvent; // 方块接受玩家互动
ll::event::ListenerPtr mPlayerUseItemEvent;       // 玩家使用物品
ll::event::ListenerPtr mArmorStandSwapItemEvent;  // 玩家交换盔甲架物品
ll::event::ListenerPtr mPlayerAttackBlockEvent;   // 玩家攻击方块
ll::event::ListenerPtr mPlayerDropItemEvent;      // 玩家丢弃物品


namespace land {
bool PreCheck(LandDataPtr ptr, UUIDs uuid, bool ignoreOperator = false) {
    if (!ignoreOperator && PLand::getInstance().isOperator(uuid)) {
        return true; // 是管理员
    } else if (!ptr) {
        return true; // 此位置没有领地
    } else if (ptr->getPermType(uuid) != LandPermType::Guest) {
        return true; // 有权限 (主人/成员)
    }
    return false;
}


bool EventListener::setup() {
    auto* db     = &PLand::getInstance();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &my_mod::MyMod::getInstance().getSelf().getLogger();

    mPlayerDestroyBlockEvent =
        bus->emplaceListener<ll::event::PlayerDestroyBlockEvent>([db, logger](ll::event::PlayerDestroyBlockEvent& ev) {
            auto& player   = ev.self();
            auto& blockPos = ev.pos();

            logger->debug("[DestroyBlock] {}", blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto& tab = land->getLandPermTableConst();
            if (tab.allowDestroy) {
                return true;
            }

            ev.cancel();
            return true;
        });

    mPlayerPlaceingBlockEvent =
        bus->emplaceListener<ll::event::PlayerPlacingBlockEvent>([db, logger](ll::event::PlayerPlacingBlockEvent& ev) {
            auto&       player   = ev.self();
            auto const& blockPos = mc::face2Pos(ev.pos(), ev.face()); // 计算实际放置位置

            logger->debug("[PlaceingBlock] {}", blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto& tab = land->getLandPermTableConst();
            if (tab.allowPlace) {
                return true;
            }

            ev.cancel();
            return true;
        });

    mPlayerUseItemOnEvent =
        bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db,
                                                                   logger](ll::event::PlayerInteractBlockEvent& ev) {
            auto& player = ev.self();
            auto& vec3   = ev.clickPos();
            auto& block  = ev.block()->getTypeName();
            auto  item   = ev.item().getTypeName();

            logger->debug(
                "[UseItemOn] Pos: {}, Item: {}, Block: {}",
                vec3.toString(),
                ev.item().getTypeName(),
                ev.block()->getTypeName()
            );

            auto land = db->getLandAt(vec3, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (!UseItemOnMap.contains(item) && !UseItemOnMap.contains(block)) {
                return true;
            }

            // clang-format off
            auto const& tab = land->getLandPermTableConst();
            if (item.ends_with( "bucket") && tab.useBucket) return true;    // 各种桶
            if (item.ends_with( "axe") && tab.allowAxePeeled) return true; // 斧头给木头去皮
            if (item.ends_with( "hoe") && tab.useHoe) return true;         // 锄头耕地
            if (item.ends_with( "_shovel") && tab.useShovel) return true;  // 锹铲除草径
            if (item == "minecraft:skull" && tab.allowPlace) return true;           // 放置头颅
            if (item == "minecraft:banner" && tab.allowPlace) return true;          // 放置旗帜
            if (item == "minecraft:glow_ink_sac" && tab.allowPlace) return true;    // 发光墨囊给木牌上色
            if (item == "minecraft:end_crystal" && tab.allowPlace) return true;     // 末地水晶
            if (item == "minecraft:ender_eye" && tab.allowPlace) return true;       // 放置末影之眼
            if (item == "minecraft:flint_and_steel" && tab.useFiregen) return true; // 使用打火石
            if (item == "minecraft:bone_meal" && tab.useBoneMeal) return true;      // 使用骨粉
            if (item == "minecraft:minecart"&& tab.allowPlace) return true; // 放置矿车
            if (item == "minecraft:armor_stand"&& tab.allowPlace) return true; // 放置矿车

            if (block.ends_with( "button") && tab.useButton) return true;       // 各种按钮
            if (block.ends_with( "_door") && tab.useDoor) return true;            // 各种门
            if (block.ends_with( "fence_gate") && tab.useFenceGate) return true;  // 各种栏栅门
            if (block.ends_with( "trapdoor") && tab.useTrapdoor) return true;     // 各种活板门
            if (block.ends_with( "_sign") && tab.editSign) return true; // 编辑告示牌
            if (block.ends_with("shulker_box") && tab.useShulkerBox) return true;  // 潜影盒
            if (block == "minecraft:dragon_egg" && tab.allowAttackDragonEgg) return true; // 右键龙蛋
            if (block == "minecraft:bed" && tab.useBed) return true;                      // 床
            if ((block == "minecraft:chest" || block == "minecraft:trapped_chest") && tab.allowOpenChest) return true; // 箱子&陷阱箱
            if (block == "minecraft:crafting_table" && tab.useCraftingTable) return true; // 工作台
            if ((block == "minecraft:campfire" || block == "minecraft:soul_campfire") && tab.useCampfire) return true; // 营火（烧烤）
            if (block == "minecraft:composter" && tab.useComposter) return true; // 堆肥桶（放置肥料）
            if (block == "minecraft:noteblock" && tab.useNoteBlock) return true; // 音符盒（调音）
            if (block == "minecraft:jukebox" && tab.useJukebox) return true;     // 唱片机（放置/取出唱片）
            if (block == "minecraft:bell" && tab.useBell) return true;           // 钟（敲钟）
            if ((block == "minecraft:daylight_detector_inverted" || block == "minecraft:daylight_detector") && tab.useDaylightDetector) return true; // 光线传感器（切换日夜模式）
            if (block == "minecraft:lectern" && tab.useLectern) return true;                // 讲台
            if (block == "minecraft:cauldron" && tab.useCauldron) return true;              // 炼药锅
            if (block == "minecraft:lever" && tab.useLever) return true;                    // 拉杆
            if (block == "minecraft:respawn_anchor" && tab.useRespawnAnchor) return true;   // 重生锚（充能）
            if (block == "minecraft:flower_pot" && tab.editFlowerPot) return true;          // 花盆
            // clang-format on

            ev.cancel();
            return true;
        });

    mFireSpreadEvent = bus->emplaceListener<ll::event::FireSpreadEvent>([db](ll::event::FireSpreadEvent& ev) {
        auto& pos = ev.pos();

        auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
        if (PreCheck(land, "")) {
            return true;
        }

        if (land->getLandPermTableConst().allowFireSpread) {
            return true;
        }

        ev.cancel();
        return true;
    });

    mPlayerAttackEntityEvent =
        bus->emplaceListener<ll::event::PlayerAttackEvent>([db, logger](ll::event::PlayerAttackEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.target().getPosition();

            logger->debug("[AttackEntity] Entity: {}, Pos: {}", ev.target().getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto const& et  = ev.target().getTypeName();
            auto const& tab = land->getLandPermTableConst();
            if (et == "minecraft:ender_crystal" && tab.allowAttackEnderCrystal) return true; // 末影水晶
            if (et == "minecraft:armor_stand" && tab.allowDestroyArmorStand) return true;    // 盔甲架
            if (tab.allowAttackPlayer && ev.target().isPlayer()) return true;                // 玩家
            if (tab.allowAttackAnimal && AnimalEntityMap.contains(et)) return true;          // 动物
            if (tab.allowAttackMob && MobEntityMap.contains(et)) return true;                // 怪物

            ev.cancel();
            return true;
        });

    mPlayerPickUpItemEvent =
        bus->emplaceListener<ll::event::PlayerPickUpItemEvent>([db, logger](ll::event::PlayerPickUpItemEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.itemActor().getPosition();

            logger->debug("[PickUpItem] Item: {}, Pos: {}", ev.itemActor().getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (land->getLandPermTableConst().allowPickupItem) return true;

            ev.cancel();
            return true;
        });

    mPlayerInteractBlockEvent =
        bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db,
                                                                   logger](ll::event::PlayerInteractBlockEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.blockPos(); // 交互的方块位置
            auto& block  = ev.block()->getTypeName();

            logger->debug("[InteractBlock]: ", pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }
            if (!InteractBlockMap.contains(block)) {
                return true;
            }

            auto const& tab = land->getLandPermTableConst();
            if (block == "minecraft:cartography_table" && tab.useCartographyTable) return true; // 制图台
            if (block == "minecraft:smithing_table" && tab.useSmithingTable) return true;       // 锻造台
            if (block == "minecraft:brewing_stand" && tab.useBrewingStand) return true;         // 酿造台
            if (block == "minecraft:anvil" && tab.useAnvil) return true;                        // 铁砧
            if (block == "minecraft:grindstone" && tab.useGrindstone) return true;              // 磨石
            if (block == "minecraft:enchanting_table" && tab.useEnchantingTable) return true;   // 附魔台
            if (block == "minecraft:barrel" && tab.useBarrel) return true;                      // 桶
            if (block == "minecraft:beacon" && tab.useBeacon) return true;                      // 信标
            if (block == "minecraft:hopper" && tab.useHopper) return true;                      // 漏斗
            if (block == "minecraft:dropper" && tab.useDropper) return true;                    // 投掷器
            if (block == "minecraft:dispenser" && tab.useDispenser) return true;                // 发射器
            if (block == "minecraft:loom" && tab.useLoom) return true;                          // 织布机
            if (block == "minecraft:stonecutter_block" && tab.useStonecutter) return true;      // 切石机
            if (block.ends_with("blast_furnace") && tab.useBlastFurnace) return true;           // 高炉
            if (block.ends_with("furnace") && tab.useFurnace) return true;                      // 熔炉
            if (block.ends_with("smoker") && tab.useSmoker) return true;                        // 烟熏炉

            ev.cancel();
            return true;
        });

    mPlayerUseItemEvent =
        bus->emplaceListener<ll::event::PlayerUseItemEvent>([db, logger](ll::event::PlayerUseItemEvent& ev) {
            if (!ev.item().getTypeName().ends_with("bucket")) {
                return true;
            }

            auto& player = ev.self();
            auto  val    = player.traceRay(5.5f, false, true, [&](BlockSource const&, Block const& bl, bool) {
                if (bl.getMaterial().isLiquid()) return false; // 液体方块
                return true;
            });

            BlockPos const&  pos   = val.mBlockPos;
            ItemStack const& item  = ev.item();
            Block const&     block = player.getDimensionBlockSource().getBlock(pos);

            logger->debug(
                "[UseItem] Item: {}, Pos: {}, Block: {}",
                item.getTypeName(),
                pos.toString(),
                block.getTypeName()
            );

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            // 防止玩家在可含水方块里放水
            if (block.getLegacyBlock().canContainLiquid()) {
                ev.cancel();
                UpdateBlockPacket(
                    pos,
                    (uint)UpdateBlockPacket::BlockLayer::Extra,
                    block.getBlockItemId(),
                    (uchar)BlockUpdateFlag::All
                )
                    .sendTo(player);
                return true;
            };
            return true;
        });

    mPlayerAttackBlockEvent =
        bus->emplaceListener<more_events::PlayerAttackBlockEvent>([db,
                                                                   logger](more_events::PlayerAttackBlockEvent& ev) {
            optional_ref<Player> pl = ev.getPlayer();
            if (!pl.has_value()) return true;

            Player& player = pl.value();

            logger->debug("[AttackBlock] {}", ev.getPos().toString());

            auto land = db->getLandAt(ev.getPos(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto const& bl = player.getDimensionBlockSourceConst().getBlock(ev.getPos()).getTypeName();
            if (land->getLandPermTableConst().allowAttackDragonEgg && bl == "minecraft:dragon_egg") return true;

            ev.cancel();
            return true;
        });

    mArmorStandSwapItemEvent =
        bus->emplaceListener<more_events::ArmorStandSwapItemEvent>([db,
                                                                    logger](more_events::ArmorStandSwapItemEvent& ev) {
            Player& player = ev.getPlayer();

            logger->debug("[ArmorStandSwapItem]: executed");

            auto land = db->getLandAt(ev.getArmorStand().getPosition(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (land->getLandPermTableConst().useArmorStand) return true;

            ev.cancel();
            return true;
        });

    mPlayerDropItemEvent =
        bus->emplaceListener<more_events::PlayerDropItemEvent>([db, logger](more_events::PlayerDropItemEvent& ev) {
            Player& player = ev.getPlayer();

            logger->debug("[PlayerDropItem]: executed");

            auto land = db->getLandAt(player.getPosition(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (land->getLandPermTableConst().allowDropItem) return true;

            return true;
        });

    return true;
}


bool EventListener::release() {
    auto& bus = ll::event::EventBus::getInstance();

    bus.removeListener(mPlayerDestroyBlockEvent);
    bus.removeListener(mPlayerPlaceingBlockEvent);
    bus.removeListener(mPlayerUseItemOnEvent);
    bus.removeListener(mFireSpreadEvent);
    bus.removeListener(mPlayerAttackEntityEvent);
    bus.removeListener(mPlayerPickUpItemEvent);
    bus.removeListener(mPlayerInteractBlockEvent);
    bus.removeListener(mPlayerUseItemEvent);
    bus.removeListener(mArmorStandSwapItemEvent);
    bus.removeListener(mPlayerAttackBlockEvent);
    bus.removeListener(mPlayerDropItemEvent);

    return true;
}


// static
std::unordered_set<string> EventListener::UseItemOnMap = {
    "minecraft:bed",                        // 床
    "minecraft:chest",                      // 箱子
    "minecraft:trapped_chest",              // 陷阱箱
    "minecraft:crafting_table",             // 制作台
    "minecraft:campfire",                   // 营火
    "minecraft:soul_campfire",              // 灵魂营火
    "minecraft:composter",                  // 垃圾箱
    "minecraft:noteblock",                  // 音符盒
    "minecraft:jukebox",                    // 唱片机
    "minecraft:bell",                       // 钟
    "minecraft:daylight_detector",          // 阳光探测器
    "minecraft:daylight_detector_inverted", // 阳光探测器(夜晚)
    "minecraft:lectern",                    // 讲台
    "minecraft:cauldron",                   // 炼药锅
    "minecraft:lever",                      // 拉杆
    "minecraft:dragon_egg",                 // 龙蛋
    "minecraft:flower_pot",                 // 花盆
    "minecraft:respawn_anchor",             // 重生锚
    "minecraft:glow_ink_sac",               // 荧光墨囊
    "minecraft:end_crystal",                // 末地水晶
    "minecraft:ender_eye",                  // 末影之眼
    "minecraft:flint_and_steel",            // 打火石
    "minecraft:skull",                      // 头颅
    "minecraft:banner",                     // 旗帜
    "minecraft:bone_meal",                  // 骨粉
    "minecraft:minecart",                   // 矿车
    "minecraft:armor_stand",                // 盔甲架

    "minecraft:axolotl_bucket",       // 美西螈桶
    "minecraft:powder_snow_bucket",   // 细雪桶
    "minecraft:pufferfish_bucket",    // 河豚桶
    "minecraft:tropical_fish_bucket", // 热带鱼桶
    "minecraft:salmon_bucket",        // 桶装鲑鱼
    "minecraft:cod_bucket",           // 鳕鱼桶
    "minecraft:water_bucket",         // 水桶
    "minecraft:cod_bucket",           // 鳕鱼桶
    "minecraft:lava_bucket",          // 熔岩桶
    "minecraft:bucket",               // 桶

    "minecraft:shulker_box",            // 潜影盒
    "minecraft:undyed_shulker_box",     // 未染色的潜影盒
    "minecraft:white_shulker_box",      // 白色潜影盒
    "minecraft:orange_shulker_box",     // 橙色潜影盒
    "minecraft:magenta_shulker_box",    // 品红色潜影盒
    "minecraft:light_blue_shulker_box", // 浅蓝色潜影盒
    "minecraft:yellow_shulker_box",     // 黄色潜影盒
    "minecraft:lime_shulker_box",       // 黄绿色潜影盒
    "minecraft:pink_shulker_box",       // 粉红色潜影盒
    "minecraft:gray_shulker_box",       // 灰色潜影盒
    "minecraft:light_gray_shulker_box", // 浅灰色潜影盒
    "minecraft:cyan_shulker_box",       // 青色潜影盒
    "minecraft:purple_shulker_box",     // 紫色潜影盒
    "minecraft:blue_shulker_box",       // 蓝色潜影盒
    "minecraft:brown_shulker_box",      // 棕色潜影盒
    "minecraft:green_shulker_box",      // 绿色潜影盒
    "minecraft:red_shulker_box",        // 红色潜影盒
    "minecraft:black_shulker_box",      // 黑色潜影盒

    "minecraft:stone_button",               // 石头按钮
    "minecraft:wooden_button",              // 木头按钮
    "minecraft:spruce_button",              // 云杉木按钮
    "minecraft:birch_button",               // 白桦木按钮
    "minecraft:jungle_button",              // 丛林木按钮
    "minecraft:acacia_button",              // 金合欢木按钮
    "minecraft:dark_oak_button",            // 深色橡木按钮
    "minecraft:crimson_button",             // 绯红木按钮
    "minecraft:warped_button",              // 诡异木按钮
    "minecraft:polished_blackstone_button", // 磨制黑石按钮
    "minecraft:mangrove_button",            // 红树木按钮
    "minecraft:cherry_button",              // 樱花木按钮
    "minecraft:bamboo_button",              // 竹按钮

    "minecraft:trapdoor",                        // 活板门
    "minecraft:spruce_trapdoor",                 // 云杉木活板门
    "minecraft:birch_trapdoor",                  // 白桦木活板门
    "minecraft:jungle_trapdoor",                 // 丛林木活板门
    "minecraft:acacia_trapdoor",                 // 金合欢木活板门
    "minecraft:dark_oak_trapdoor",               // 深色橡木活板门
    "minecraft:crimson_trapdoor",                // 绯红木活板门
    "minecraft:warped_trapdoor",                 // 诡异木活板门
    "minecraft:copper_trapdoor",                 // 铜活板门
    "minecraft:exposed_copper_trapdoor",         // 斑驳的铜活板门
    "minecraft:weathered_copper_trapdoor",       // 锈蚀的铜活板门
    "minecraft:oxidized_copper_trapdoor",        // 氧化的铜活板门
    "minecraft:waxed_copper_trapdoor",           // 涂蜡的铜活板门
    "minecraft:waxed_exposed_copper_trapdoor",   // 涂蜡的斑驳的铜活板门
    "minecraft:waxed_weathered_copper_trapdoor", // 涂蜡的锈蚀的铜活板门
    "minecraft:waxed_oxidized_copper_trapdoor",  // 涂蜡的氧化的铜活板门
    "minecraft:mangrove_trapdoor",               // 红树木活板门
    "minecraft:cherry_trapdoor",                 // 樱树木活板门
    "minecraft:bamboo_trapdoor",                 // 竹活板门

    "minecraft:fence_gate",          // 栅栏门
    "minecraft:spruce_fence_gate",   // 云杉木栅栏门
    "minecraft:birch_fence_gate",    // 白桦木栅栏门
    "minecraft:jungle_fence_gate",   // 丛林木栅栏门
    "minecraft:acacia_fence_gate",   // 金合欢木栅栏门
    "minecraft:dark_oak_fence_gate", // 深色橡木栅栏门
    "minecraft:crimson_fence_gate",  // 绯红木栅栏门
    "minecraft:warped_fence_gate",   // 诡异木栅栏门
    "minecraft:mangrove_fence_gate", // 红树木栅栏门
    "minecraft:cherry_fence_gate",   // 樱树木栅栏门
    "minecraft:bamboo_fence_gate",   // 竹栅栏门

    "minecraft:wooden_door",   // 橡木门
    "minecraft:spruce_door",   // 云杉木门
    "minecraft:birch_door",    // 白桦木门
    "minecraft:jungle_door",   // 丛林木门
    "minecraft:acacia_door",   // 金合欢木门
    "minecraft:dark_oak_door", // 深色橡木门
    "minecraft:crimson_door",  // 绯红木门
    "minecraft:warped_door",   // 诡异木门
    "minecraft:mangrove_door", // 红树木门
    "minecraft:cherry_door",   // 樱树木门
    "minecraft:bamboo_door",   // 竹门

    "minecraft:wooden_axe",       // 木斧
    "minecraft:stone_axe",        // 石斧
    "minecraft:iron_axe",         // 铁斧
    "minecraft:golden_axe",       // 金斧
    "minecraft:diamond_axe",      // 钻石斧
    "minecraft:netherite_axe",    // 下界合金斧
    "minecraft:wooden_hoe",       // 木锄
    "minecraft:stone_hoe",        // 石锄
    "minecraft:iron_hoe",         // 铁锄
    "minecraft:diamond_hoe",      // 钻石锄
    "minecraft:golden_hoe",       // 金锄
    "minecraft:netherite_hoe",    // 下界合金锄
    "minecraft:wooden_shovel",    // 木铲
    "minecraft:stone_shovel",     // 石铲
    "minecraft:iron_shovel",      // 铁铲
    "minecraft:diamond_shovel",   // 钻石铲
    "minecraft:golden_shovel",    // 金铲
    "minecraft:netherite_shovel", // 下界合金铲

    "minecraft:standing_sign",          // 站立的告示牌
    "minecraft:spruce_standing_sign",   // 站立的云杉木告示牌
    "minecraft:birch_standing_sign",    // 站立的白桦木告示牌
    "minecraft:jungle_standing_sign",   // 站立的丛林木告示牌
    "minecraft:acacia_standing_sign",   // 站立的金合欢木告示牌
    "minecraft:darkoak_standing_sign",  // 站立的深色橡木告示牌
    "minecraft:mangrove_standing_sign", // 站立的红树木告示牌
    "minecraft:cherry_standing_sign",   // 站立的樱树木告示牌
    "minecraft:bamboo_standing_sign",   // 站立的竹子告示牌
    "minecraft:crimson_standing_sign",  // 站立的绯红木告示牌
    "minecraft:warped_standing_sign",   // 站立的诡异木告示牌
    "minecraft:wall_sign",              // 墙上的告示牌
    "minecraft:spruce_wall_sign",       // 墙上的云杉木告示牌
    "minecraft:birch_wall_sign",        // 墙上的白桦木告示牌
    "minecraft:jungle_wall_sign",       // 墙上的丛林木告示牌
    "minecraft:acacia_wall_sign",       // 墙上的金合欢木告示牌
    "minecraft:darkoak_wall_sign",      // 墙上的深色橡木告示牌
    "minecraft:mangrove_wall_sign",     // 墙上的红树木告示牌
    "minecraft:cherry_wall_sign",       // 墙上的樱树木告示牌
    "minecraft:bamboo_wall_sign",       // 墙上的竹子告示牌
    "minecraft:crimson_wall_sign",      // 墙上的绯红木告示牌
    "minecraft:warped_wall_sign"        // 墙上的诡异木告示牌
};
std::unordered_set<string> EventListener::InteractBlockMap = {
    "minecraft:cartography_table", // 制图台
    "minecraft:smithing_table",    // 锻造台
    "minecraft:furnace",           // 熔炉
    "minecraft:blast_furnace",     // 高炉
    "minecraft:smoker",            // 烟熏炉
    "minecraft:brewing_stand",     // 酿造台
    "minecraft:anvil",             // 铁砧
    "minecraft:grindstone",        // 砂轮
    "minecraft:enchanting_table",  // 附魔台
    "minecraft:barrel",            // 木桶
    "minecraft:beacon",            // 信标
    "minecraft:hopper",            // 漏斗
    "minecraft:dropper",           // 投掷器
    "minecraft:dispenser",         // 发射器
    "minecraft:loom",              // 织布机
    "minecraft:stonecutter_block", // 切石机
    "minecraft:lit_furnace",       // 燃烧中的熔炉
    "minecraft:lit_blast_furnace", // 燃烧中的高炉
    "minecraft:lit_smoker"         // 燃烧中的烟熏炉
};
std::unordered_set<string> EventListener::AnimalEntityMap = {
    "minecraft:axolotl",          // 美西螈
    "minecraft:bat",              // 蝙蝠
    "minecraft:cat",              // 猫
    "minecraft:chicken",          // 鸡
    "minecraft:cod",              // 鳕鱼
    "minecraft:cow",              // 牛
    "minecraft:donkey",           // 驴
    "minecraft:fox",              // 狐狸
    "minecraft:glow_squid",       // 发光鱿鱼
    "minecraft:horse",            // 马
    "minecraft:mooshroom",        // 蘑菇牛
    "minecraft:mule",             // 驴
    "minecraft:ocelot",           // 豹猫
    "minecraft:parrot",           // 鹦鹉
    "minecraft:pig",              // 猪
    "minecraft:rabbit",           // 兔子
    "minecraft:salmon",           // 鲑鱼
    "minecraft:snow_golem",       // 雪傀儡
    "minecraft:sheep",            // 羊
    "minecraft:skeleton_horse",   // 骷髅马
    "minecraft:squid",            // 鱿鱼
    "minecraft:strider",          // 炽足兽
    "minecraft:tropical_fish",    // 热带鱼
    "minecraft:turtle",           // 海龟
    "minecraft:villager_v2",      // 村民
    "minecraft:wandering_trader", // 流浪商人
    "minecraft:npc"               // NPC
};
std::unordered_set<string> EventListener::MobEntityMap = {
    "minecraft:pufferfish",    // 河豚
    "minecraft:bee",           // 蜜蜂
    "minecraft:dolphin",       // 海豚
    "minecraft:goat",          // 山羊
    "minecraft:iron_golem",    // 铁傀儡
    "minecraft:llama",         // 羊驼
    "minecraft:llama_spit",    // 羊驼唾沫
    "minecraft:wolf",          // 狼
    "minecraft:panda",         // 熊猫
    "minecraft:polar_bear",    // 北极熊
    "minecraft:enderman",      // 末影人
    "minecraft:piglin",        // 猪灵
    "minecraft:spider",        // 蜘蛛
    "minecraft:cave_spider",   // 洞穴蜘蛛
    "minecraft:zombie_pigman", // 僵尸猪人

    "minecraft:blaze",                 // 烈焰人
    "minecraft:small_fireball",        // 小火球
    "minecraft:creeper",               // 爬行者
    "minecraft:drowned",               // 溺尸
    "minecraft:elder_guardian",        // 远古守卫者
    "minecraft:husk",                  // 凋零骷髅
    "minecraft:skeleton",              // 骷髅
    "minecraft:skeleton_horse",        // 骷髅马
    "minecraft:slime",                 // 史莱姆
    "minecraft:vex",                   // 恶魂兽
    "minecraft:vindicator",            // 卫道士
    "minecraft:witch",                 // 女巫
    "minecraft:endermite",             // 末影螨
    "minecraft:evocation_illager",     // 唤魔者
    "minecraft:evocation_fang",        // 唤魔者尖牙
    "minecraft:magma_cube",            // 岩浆怪
    "minecraft:phantom",               // 幻翼
    "minecraft:pillager",              // 掠夺者
    "minecraft:ravager",               // 劫掠兽
    "minecraft:shulker",               // 潜影贝
    "minecraft:shulker_bullet",        // 潜影贝弹射物
    "minecraft:silverfish",            // 蠹虫
    "minecraft:ghast",                 // 恶魂
    "minecraft:fireball",              // 火球
    "minecraft:guardian",              // 守卫者
    "minecraft:hoglin",                // 疣猪兽
    "minecraft:wither_skeleton",       // 凋零骷髅
    "minecraft:zoglin",                // 疣猪兽
    "minecraft:zombie",                // 僵尸
    "minecraft:zombie_villager_v2",    // 僵尸村民
    "minecraft:piglin_brute",          // 猪灵蛮兵
    "minecraft:ender_dragon",          // 末影龙
    "minecraft:dragon_fireball",       // 末影龙火球
    "minecraft:wither",                // 凋零
    "minecraft:wither_skull",          // 凋零之首
    "minecraft:wither_skull_dangerous" // 蓝色凋灵之首(Wiki)
};


} // namespace land
