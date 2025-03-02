#include "pland/EventListener.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/world/SpawnMobEvent.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/network/packet/UpdateBlockPacket.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/material/Material.h"
#include "mc/world/phys/AABB.h"
#include "mc/world/phys/HitResult.h"
#include "mc\world\level\block\components\BlockLiquidDetectionComponent.h"
#include "mc\world\level\chunk\SubChunk.h"
#include "mod/MyMod.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include "pland/utils/MC.h"
#include <functional>
#include <unordered_set>
#include <vector>


#include "ll/api/event/entity/ActorHurtEvent.h"
#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/event/player/PlayerDestroyBlockEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/event/player/PlayerPickUpItemEvent.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "ll/api/event/player/PlayerUseItemEvent.h"
#include "ll/api/event/world/FireSpreadEvent.h"

#include "ila/event/minecraft/world/ExplosionEvent.h"
#include "ila/event/minecraft/world/PistonPushEvent.h"
#include "ila/event/minecraft/world/RedstoneUpdateEvent.h"
#include "ila/event/minecraft/world/SculkBlockGrowthEvent.h"
#include "ila/event/minecraft/world/WitherDestroyEvent.h"
#include "ila/event/minecraft/world/actor/ActorRideEvent.h"
#include "ila/event/minecraft/world/actor/ActorTriggerPressurePlateEvent.h"
#include "ila/event/minecraft/world/actor/ArmorStandSwapItemEvent.h"
#include "ila/event/minecraft/world/actor/MobHurtEffectEvent.h"
#include "ila/event/minecraft/world/actor/ProjectileCreateEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerAttackBlockEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerDropItemEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerEditSignEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerOperatedItemFrameEvent.h"
#include "ila/event/minecraft/world/level/block/FarmDecayEvent.h"
#include "ila/event/minecraft/world/level/block/LiquidTryFlowEvent.h"
#include "ila/event/minecraft/world/level/block/MossGrowthEvent.h"
#include "ila/event/minecraft/world/level/block/SculkCatalystAbsorbExperienceEvent.h"
#include "ila/event/minecraft/world/level/block/SculkSpreadEvent.h"


static std::vector<ll::event::ListenerPtr> mListeners;

namespace land {
inline bool PreCheck(LandData_sptr const& ptr, UUIDs const& uuid = "", bool ignoreOperator = false) {
    if (!ptr ||                                                       // 无领地
        (!ignoreOperator && PLand::getInstance().isOperator(uuid)) || // 管理员
        (ptr->getPermType(uuid) != LandPermType::Guest)               // 主人/成员
    ) {
        return true;
    }
    return false;
}


bool EventListener::setup() {
    auto* db     = &PLand::getInstance();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &my_mod::MyMod::getInstance().getSelf().getLogger();

    mListeners = {
        // LL
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
        bus->emplaceListener<ll::event::ActorHurtEvent>([db, logger](ll::event::ActorHurtEvent& ev) {
            if (!Config::cfg.listeners.ActorHurtEvent) return;

            auto& self   = ev.self();
            auto& source = ev.source();
            logger->debug(
                "[ActorHurt] Mob: {}, ActorDamageCause: {}, ActorType: {}",
                self.getTypeName(),
                static_cast<int>(source.getCause()),
                static_cast<int>(source.getEntityType())
            );

            auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
            if (PreCheck(land)) return; // land not found

            if (source.getEntityType() == ActorType::Player && source.getCause() == ActorDamageCause::EntityAttack) {
                // 玩家攻击 [ActorHurt] Mob: ikun, ActorDamageCause: 2, ActorType: 319
                if (auto souPlayer = self.getILevel().getPlayer(source.getEntityUniqueID()); souPlayer) {
                    if (PreCheck(land, souPlayer->getUuid().asString())) return;
                }
            }

            if (land) {
                auto const& tab = land->getLandPermTableConst();
                if (tab.allowAttackPlayer && self.isPlayer()) return;
                if (tab.allowAttackAnimal && self.hasCategory(::ActorCategory::Animal)) return;
                if (tab.allowAttackMonster && self.hasCategory(::ActorCategory::Monster)) return;
            }

            if (self.isPlayer()) {
                auto const pl = self.getWeakEntity().tryUnwrap<Player>();
                if (pl.has_value()) {
                    if (PreCheck(land, pl->getUuid().asString())) return;
                }
            }

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::PlayerDestroyBlockEvent>([db, logger](ll::event::PlayerDestroyBlockEvent& ev) {
            if (!Config::cfg.listeners.PlayerDestroyBlockEvent) return;

            auto& player   = ev.self();
            auto& blockPos = ev.pos();

            logger->debug("[DestroyBlock] {}", blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            auto& tab = land->getLandPermTableConst();
            if (tab.allowDestroy) {
                return;
            }

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::PlayerPlacingBlockEvent>([db, logger](ll::event::PlayerPlacingBlockEvent& ev) {
            if (!Config::cfg.listeners.PlayerPlacingBlockEvent) return;

            auto&       player   = ev.self();
            auto const& blockPos = mc::face2Pos(ev.pos(), ev.face()); // 计算实际放置位置

            logger->debug("[PlaceingBlock] {}", blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            auto& tab = land->getLandPermTableConst();
            if (tab.allowPlace) {
                return;
            }

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db,
                                                                   logger](ll::event::PlayerInteractBlockEvent& ev) {
            if (!Config::cfg.listeners.PlayerInteractBlockEvent) return;

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
                return;
            }

            if (!UseItemOnMap.contains(item) && !UseItemOnMap.contains(block)) {
                return;
            }

            // clang-format off
            auto const& tab = land->getLandPermTableConst();
            if (item.ends_with( "bucket") && tab.useBucket) return;    // 各种桶
            if (item.ends_with( "axe") && tab.allowAxePeeled) return; // 斧头给木头去皮
            if (item.ends_with( "hoe") && tab.useHoe) return;         // 锄头耕地
            if (item.ends_with( "_shovel") && tab.useShovel) return;  // 锹铲除草径
            if (item == "minecraft:skull" && tab.allowPlace) return;           // 放置头颅
            if (item == "minecraft:banner" && tab.allowPlace) return;          // 放置旗帜
            if (item == "minecraft:glow_ink_sac" && tab.allowPlace) return;    // 发光墨囊给木牌上色
            if (item == "minecraft:end_crystal" && tab.allowPlace) return;     // 末地水晶
            if (item == "minecraft:ender_eye" && tab.allowPlace) return;       // 放置末影之眼
            if (item == "minecraft:flint_and_steel" && tab.useFiregen) return; // 使用打火石
            if (item == "minecraft:bone_meal" && tab.useBoneMeal) return;      // 使用骨粉
            if (item == "minecraft:minecart"&& tab.allowPlace) return; // 放置矿车
            if (item == "minecraft:armor_stand"&& tab.allowPlace) return; // 放置矿车

            if (block.ends_with( "button") && tab.useButton) return;       // 各种按钮
            if (block.ends_with( "_door") && tab.useDoor) return;            // 各种门
            if (block.ends_with( "fence_gate") && tab.useFenceGate) return;  // 各种栏栅门
            if (block.ends_with( "trapdoor") && tab.useTrapdoor) return;     // 各种活板门
            if (block.ends_with( "_sign") && tab.editSign) return; // 编辑告示牌
            if (block.ends_with("shulker_box") && tab.useShulkerBox) return;  // 潜影盒
            if (block == "minecraft:dragon_egg" && tab.allowAttackDragonEgg) return; // 右键龙蛋
            if (block == "minecraft:bed" && tab.useBed) return;                      // 床
            if ((block == "minecraft:chest" || block == "minecraft:trapped_chest") && tab.allowOpenChest) return; // 箱子&陷阱箱
            if (block == "minecraft:crafting_table" && tab.useCraftingTable) return; // 工作台
            if ((block == "minecraft:campfire" || block == "minecraft:soul_campfire") && tab.useCampfire) return; // 营火（烧烤）
            if (block == "minecraft:composter" && tab.useComposter) return; // 堆肥桶（放置肥料）
            if (block == "minecraft:noteblock" && tab.useNoteBlock) return; // 音符盒（调音）
            if (block == "minecraft:jukebox" && tab.useJukebox) return;     // 唱片机（放置/取出唱片）
            if (block == "minecraft:bell" && tab.useBell) return;           // 钟（敲钟）
            if ((block == "minecraft:daylight_detector_inverted" || block == "minecraft:daylight_detector") && tab.useDaylightDetector) return; // 光线传感器（切换日夜模式）
            if (block == "minecraft:lectern" && tab.useLectern) return;                // 讲台
            if (block == "minecraft:cauldron" && tab.useCauldron) return;              // 炼药锅
            if (block == "minecraft:lever" && tab.useLever) return;                    // 拉杆
            if (block == "minecraft:respawn_anchor" && tab.useRespawnAnchor) return;   // 重生锚（充能）
            if (block == "minecraft:flower_pot" && tab.editFlowerPot) return;          // 花盆
            // clang-format on

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::FireSpreadEvent>([db](ll::event::FireSpreadEvent& ev) {
            if (!Config::cfg.listeners.FireSpreadEvent) return;

            auto& pos = ev.pos();

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (PreCheck(land)) {
                return;
            }

            if (land->getLandPermTableConst().allowFireSpread) {
                return;
            }

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::PlayerAttackEvent>([db, logger](ll::event::PlayerAttackEvent& ev) {
            if (!Config::cfg.listeners.PlayerAttackEvent) return;

            auto& player = ev.self();
            auto& mob    = ev.target();
            auto& pos    = mob.getPosition();

            logger->debug("[AttackEntity] Entity: {}, Pos: {}", mob.getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            auto const& et  = mob.getTypeName();
            auto const& tab = land->getLandPermTableConst();
            if (et == "minecraft:ender_crystal" && tab.allowAttackEnderCrystal) return;      // 末影水晶
            if (et == "minecraft:armor_stand" && tab.allowDestroyArmorStand) return;         // 盔甲架
            if (tab.allowAttackPlayer && mob.isPlayer()) return;                             // 玩家
            if (tab.allowAttackAnimal && mob.hasCategory(::ActorCategory::Animal)) return;   // 动物
            if (tab.allowAttackMonster && mob.hasCategory(::ActorCategory::Monster)) return; // 怪物

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::PlayerPickUpItemEvent>([db, logger](ll::event::PlayerPickUpItemEvent& ev) {
            if (!Config::cfg.listeners.PlayerPickUpItemEvent) return;

            auto& player = ev.self();
            auto& pos    = ev.itemActor().getPosition();

            logger->debug("[PickUpItem] Item: {}, Pos: {}", ev.itemActor().getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            if (land->getLandPermTableConst().allowPickupItem) return;

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db,
                                                                   logger](ll::event::PlayerInteractBlockEvent& ev) {
            if (!Config::cfg.listeners.PlayerInteractBlockEvent) return;

            auto& player = ev.self();
            auto& pos    = ev.blockPos(); // 交互的方块位置
            auto& block  = ev.block()->getTypeName();

            logger->debug("[InteractBlock]: ", pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }
            if (!InteractBlockMap.contains(block)) {
                return;
            }

            auto const& tab = land->getLandPermTableConst();
            if (block == "minecraft:cartography_table" && tab.useCartographyTable) return; // 制图台
            if (block == "minecraft:smithing_table" && tab.useSmithingTable) return;       // 锻造台
            if (block == "minecraft:brewing_stand" && tab.useBrewingStand) return;         // 酿造台
            if (block == "minecraft:anvil" && tab.useAnvil) return;                        // 铁砧
            if (block == "minecraft:grindstone" && tab.useGrindstone) return;              // 磨石
            if (block == "minecraft:enchanting_table" && tab.useEnchantingTable) return;   // 附魔台
            if (block == "minecraft:barrel" && tab.useBarrel) return;                      // 桶
            if (block == "minecraft:beacon" && tab.useBeacon) return;                      // 信标
            if (block == "minecraft:hopper" && tab.useHopper) return;                      // 漏斗
            if (block == "minecraft:dropper" && tab.useDropper) return;                    // 投掷器
            if (block == "minecraft:dispenser" && tab.useDispenser) return;                // 发射器
            if (block == "minecraft:loom" && tab.useLoom) return;                          // 织布机
            if (block == "minecraft:stonecutter_block" && tab.useStonecutter) return;      // 切石机
            if (block.ends_with("blast_furnace") && tab.useBlastFurnace) return;           // 高炉
            if (block.ends_with("furnace") && tab.useFurnace) return;                      // 熔炉
            if (block.ends_with("smoker") && tab.useSmoker) return;                        // 烟熏炉

            ev.cancel();
        }),
        bus->emplaceListener<ll::event::PlayerUseItemEvent>([db, logger](ll::event::PlayerUseItemEvent& ev) {
            if (!Config::cfg.listeners.PlayerUseItemEvent) return;

            if (!ev.item().getTypeName().ends_with("bucket")) {
                return;
            }

            auto& player = ev.self();
            auto  val    = player.traceRay(5.5f, false, true, [&](BlockSource const&, Block const& bl, bool) {
                if (bl.getMaterial().isLiquid()) return false; // 液体方块
                return true;
            });

            BlockPos const&  pos   = val.mBlock;
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
                return;
            }

            // 防止玩家在可含水方块里放水
            if (BlockLiquidDetectionComponent::canContainLiquid(block)) {
                ev.cancel();
                static uchar flags = (1 << 0) | (1 << 1); // 0b11 BlockUpdateFlag::All v0.13.5
                UpdateBlockPacket(pos, (uint)SubChunk::BlockLayer::Extra, block.getBlockItemId(), flags).sendTo(player);
            };
        }),
        // ila
        bus->emplaceListener<ila::mc::PlayerAttackBlockBeforeEvent>(
            [db, logger](ila::mc::PlayerAttackBlockBeforeEvent& ev) {
                if (!Config::cfg.listeners.PlayerAttackBlockBeforeEvent) return;

                optional_ref<Player> pl = ev.self();
                if (!pl.has_value()) return;

                Player& player = pl.value();

                logger->debug("[AttackBlock] {}", ev.getPos().toString());

                auto land = db->getLandAt(ev.getPos(), player.getDimensionId());
                if (PreCheck(land, player.getUuid().asString())) {
                    return;
                }

                auto const& bl = player.getDimensionBlockSourceConst().getBlock(ev.getPos()).getTypeName();
                if (land->getLandPermTableConst().allowAttackDragonEgg && bl == "minecraft:dragon_egg") return;

                ev.cancel();
            }
        ),
        bus->emplaceListener<ila::mc::ArmorStandSwapItemBeforeEvent>(
            [db, logger](ila::mc::ArmorStandSwapItemBeforeEvent& ev) {
                if (!Config::cfg.listeners.ArmorStandSwapItemBeforeEvent) return;

                Player& player = ev.getPlayer();

                logger->debug("[ArmorStandSwapItem]: executed");

                auto land = db->getLandAt(ev.self().getPosition(), player.getDimensionId());
                if (PreCheck(land, player.getUuid().asString())) {
                    return;
                }

                if (land->getLandPermTableConst().useArmorStand) return;

                ev.cancel();
            }
        ),
        bus->emplaceListener<ila::mc::PlayerDropItemBeforeEvent>([db, logger](ila::mc::PlayerDropItemBeforeEvent& ev) {
            if (!Config::cfg.listeners.PlayerDropItemBeforeEvent) return;

            Player& player = ev.self();

            logger->debug("[PlayerDropItem]: executed");

            auto land = db->getLandAt(player.getPosition(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            if (land->getLandPermTableConst().allowDropItem) return;

            ev.cancel();
        }),
        bus->emplaceListener<ila::mc::ActorRideBeforeEvent>([db, logger](ila::mc::ActorRideBeforeEvent& ev) {
            if (!Config::cfg.listeners.ActorRideBeforeEvent) return;

            logger->debug("[ActorRide]: executed");
            Actor& passenger = ev.self();

            if (!passenger.isPlayer()) {
                return; // 忽略非玩家骑乘事件
            }

            auto const& typeName = ev.getTarget().getTypeName();
            auto        land     = db->getLandAt(passenger.getPosition(), passenger.getDimensionId());
            if (PreCheck(land)) return; // land not found
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
                if (player.has_value() && PreCheck(land, player->getUuid().asString())) {
                    return;
                }
            }

            ev.cancel();
        }),
        bus->emplaceListener<ila::mc::ExplosionBeforeEvent>([db, logger](ila::mc::ExplosionBeforeEvent& ev) {
            if (!Config::cfg.listeners.ExplosionBeforeEvent) return;

            logger->debug("[Explode] Pos: {}", ev.getExplosion().mPos->toString());

            auto lands = db->getLandAt(
                BlockPos{ev.getExplosion().mPos},
                (int)(ev.getExplosion().mRadius + 1.0),
                ev.blockSource().getDimensionId()
            );
            for (auto& p : lands) {
                if (!p->getLandPermTableConst().allowExplode) {
                    ev.cancel();
                    break;
                }
            }
        }),
        bus->emplaceListener<ila::mc::FarmDecayBeforeEvent>([db, logger](ila::mc::FarmDecayBeforeEvent& ev) {
            if (!Config::cfg.listeners.FarmDecayBeforeEvent) return;

            logger->debug("[FarmDecay] Pos: {}", ev.getPos().toString());

            auto land = db->getLandAt(ev.getPos(), ev.blockSource().getDimensionId());
            if (PreCheck(land)) return; // land not found
            if (land) {
                if (land->getLandPermTableConst().allowFarmDecay) return;
            }

            ev.cancel();
        }),
        bus->emplaceListener<ila::mc::MobHurtEffectBeforeEvent>([db, logger](ila::mc::MobHurtEffectBeforeEvent& ev) {
            if (!Config::cfg.listeners.MobHurtEffectBeforeEvent) return;

            logger->debug("[MobHurtEffect] mob: {}", ev.self().getTypeName());
            auto& self = ev.self();

            auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
            if (PreCheck(land)) return; // land not found
            if (land) {
                auto const& tab = land->getLandPermTableConst();
                if (tab.allowAttackPlayer && self.isPlayer()) return;
                if (tab.allowAttackAnimal && self.hasCategory(::ActorCategory::Animal)) return;
                if (tab.allowAttackMonster && self.hasCategory(::ActorCategory::Monster)) return;
            }

            if (self.isPlayer()) {
                auto const pl = self.getWeakEntity().tryUnwrap<Player>();
                if (pl.has_value()) {
                    if (PreCheck(land, pl->getUuid().asString())) return;
                }
            }

            ev.cancel();
        }),
        bus->emplaceListener<ila::mc::PistonPushBeforeEvent>([db, logger](ila::mc::PistonPushBeforeEvent& ev) {
            if (!Config::cfg.listeners.PistonPushBeforeEvent) return;

            auto const& piston = ev.getPistonPos();
            auto const& push   = ev.getPushPos();
            auto&       region = ev.blockSource();

            logger->debug("[PistonTryPush] piston: {}, push: {}", piston.toString(), push.toString());

            auto land  = db->getLandAt(push, region.getDimensionId());
            auto land2 = db->getLandAt(piston, region.getDimensionId());
            if (land && !land->getLandPermTableConst().allowPistonPush && land != land2) {
                ev.cancel();
            }
        }),
        bus->emplaceListener<ila::mc::PlayerOperatedItemFrameBeforeEvent>(
            [db, logger](ila::mc::PlayerOperatedItemFrameBeforeEvent& ev) {
                if (!Config::cfg.listeners.PlayerOperatedItemFrameBeforeEvent) return;

                logger->debug("[PlayerUseItemFrame] pos: {}", ev.getBlockPos().toString());

                auto land = db->getLandAt(ev.getBlockPos(), ev.self().getDimensionId());
                if (PreCheck(land, ev.self().getUuid().asString())) return;

                if (land->getLandPermTableConst().useItemFrame) return;

                ev.cancel();
            }
        ),
        bus->emplaceListener<ila::mc::ActorTriggerPressurePlateBeforeEvent>(
            [db, logger](ila::mc::ActorTriggerPressurePlateBeforeEvent& ev) {
                if (!Config::cfg.listeners.ActorTriggerPressurePlateBeforeEvent) return;

                logger->debug("[PressurePlateTrigger] pos: {}", ev.getPos().toString());

                auto land = db->getLandAt(ev.getPos(), ev.self().getDimensionId());
                if (land && land->getLandPermTableConst().usePressurePlate) return;
                if (PreCheck(land)) return; // land not found

                auto& entity = ev.self();
                if (entity.isPlayer()) {
                    auto pl = entity.getWeakEntity().tryUnwrap<Player>();
                    if (pl.has_value()) {
                        if (PreCheck(land, pl->getUuid().asString())) return;
                    }
                }

                ev.cancel();
            }
        ),
        bus->emplaceListener<ila::mc::ProjectileCreateBeforeEvent>([db,
                                                                    logger](ila::mc::ProjectileCreateBeforeEvent& ev) {
            if (!Config::cfg.listeners.ProjectileCreateBeforeEvent) return;

            Actor& self = ev.self();
            auto&  type = ev.self().getTypeName();

            logger->debug("[ProjectileSpawn] type: {}", type);

            auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
            if (PreCheck(land)) return; // land not found

            if (self.getOwnerEntityType() == ActorType::Player) {
                // 由玩家所创建的实体
                if (auto mob = self.getOwner(); mob && mob->isPlayer()) {
                    auto pl = mob->getWeakEntity().tryUnwrap<Player>();
                    if (pl.has_value() && PreCheck(land, pl->getUuid().asString())) return;
                }
            }

            if (land) {
                auto& tab = land->getLandPermTableConst();
                if (type == "minecraft:fishing_hook" && tab.useFishingHook) return;       // 钓鱼竿
                if (type == "minecraft:splash_potion" && tab.allowThrowPotion) return;    // 喷溅药水
                if (type == "minecraft:lingering_potion" && tab.allowThrowPotion) return; // 滞留药水
                if (type == "minecraft:thrown_trident" && tab.allowThrowTrident) return;  // 三叉戟
                if (type == "minecraft:arrow" && tab.allowShoot) return;                  // 箭
                if (type == "minecraft:crossbow" && tab.allowShoot) return;               // 弩射烟花
                if (type == "minecraft:snowball" && tab.allowThrowSnowball) return;       // 雪球
                if (type == "minecraft:ender_pearl" && tab.allowThrowEnderPearl) return;  // 末影珍珠
                if (type == "minecraft:egg" && tab.allowThrowEgg) return;                 // 鸡蛋
            }

            ev.cancel();
        }),
        bus->emplaceListener<ila::mc::RedstoneUpdateBeforeEvent>([db, logger](ila::mc::RedstoneUpdateBeforeEvent& ev) {
            if (!Config::cfg.listeners.RedstoneUpdateBeforeEvent) return;

            logger->debug("[RedstoneUpdate] Pos: {}", ev.getPos().toString());

            auto land = db->getLandAt(ev.getPos(), ev.blockSource().getDimensionId());
            if (PreCheck(land)) return; // land not found
            if (land) {
                if (land->getLandPermTableConst().allowRedstoneUpdate) return;
            }

            ev.cancel();
        }),
        bus->emplaceListener<ila::mc::WitherDestroyBeforeEvent>([db, logger](ila::mc::WitherDestroyBeforeEvent& ev) {
            if (!Config::cfg.listeners.WitherDestroyBeforeEvent) return;

            logger->debug("[WitherDestroyBlock] executed");
            auto& aabb = ev.getBox();

            auto lands = db->getLandAt(aabb.min, aabb.max, ev.blockSource().getDimensionId());
            for (auto const& p : lands) {
                if (!p->getLandPermTableConst().allowWitherDestroy) {
                    ev.cancel();
                    break;
                }
            }
        }),
        bus->emplaceListener<ila::mc::MossGrowthBeforeEvent>([db, logger](ila::mc::MossGrowthBeforeEvent& ev) {
            if (!Config::cfg.listeners.MossGrowthBeforeEvent) return;

            // logger->debug("[MossSpread] {}", ev.getPos().toString());

            auto const& pos = ev.getPos();

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (!land || land->getLandPermTableConst().useBoneMeal) {
                return;
            }

            auto lds = db->getLandAt(pos - 2, pos + 2, ev.blockSource().getDimensionId());
            for (auto const& p : lds) {
                if (p->getLandPermTableConst().useBoneMeal) {
                    return;
                }
            }

            ev.cancel();
        }),
        bus->emplaceListener<ila::mc::LiquidTryFlowBeforeEvent>([db, logger](ila::mc::LiquidTryFlowBeforeEvent& ev) {
            if (!Config::cfg.listeners.LiquidTryFlowBeforeEvent) return;

            auto& sou = ev.getPos();
            // auto& from = ev.getFlowFromPos();
            // logger->debug("[LiquidFlow] {} -> {}", sou.toString(), from.toString());

            auto land = db->getLandAt(sou, ev.blockSource().getDimensionId());
            if (land && !land->getLandPermTableConst().allowLiquidFlow) {
                ev.cancel();
            }
        }),
        bus->emplaceListener<ila::mc::SculkBlockGrowthBeforeEvent>([db,
                                                                    logger](ila::mc::SculkBlockGrowthBeforeEvent& ev) {
            if (!Config::cfg.listeners.SculkBlockGrowthBeforeEvent) return;

            auto& pos = ev.getPos();
            logger->debug("[SculkBlockGrowth] {}", pos.toString());

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (land) {
                if (!land->getLandPermTableConst().allowSculkBlockGrowth) {
                    ev.cancel();
                }
            }
        }),
        bus->emplaceListener<ila::mc::SculkSpreadBeforeEvent>([db, logger](ila::mc::SculkSpreadBeforeEvent& ev) {
            if (!Config::cfg.listeners.SculkSpreadBeforeEvent) return;

            // logger->debug("[SculkSpread] {} -> {}", ev.getSelfPos().toString(), ev.getTargetPos().toString());

            auto sou = db->getLandAt(ev.getSelfPos(), ev.blockSource().getDimensionId());
            auto tar = db->getLandAt(ev.getTargetPos(), ev.blockSource().getDimensionId());

            if (!sou && !tar) return; // 领地外蔓延
            if (sou && tar) return;   // 领地内蔓延
            if (sou && !tar) return;  // 领地内蔓延到外
            if (!sou && tar) {
                ev.cancel(); // 外蔓延到领地内
            }
        }),
        bus->emplaceListener<ila::mc::PlayerEditSignBeforeEvent>([db, logger](ila::mc::PlayerEditSignBeforeEvent& ev) {
            if (!Config::cfg.listeners.PlayerEditSignBeforeEvent) return;

            auto& player = ev.self();
            auto& pos    = ev.getPos();

            logger->debug("[PlayerEditSign] {} -> {}", player.getName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            if (land && !land->getLandPermTableConst().editSign) {
                ev.cancel();
            }
        }),
        bus->emplaceListener<ila::mc::SculkCatalystAbsorbExperienceBeforeEvent>(
            [db, logger](ila::mc::SculkCatalystAbsorbExperienceBeforeEvent& ev) {
                if (!Config::cfg.listeners.SculkCatalystAbsorbExperienceBeforeEvent) return;

                // auto& actor  = ev.getDiedActor();
                auto& actor  = ev.getActor();
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
        ),
        bus->emplaceListener<ll::event::SpawnedMobEvent>([db, logger](ll::event::SpawnedMobEvent& ev) {
            if (!Config::cfg.listeners.SpawnedMobEvent) return;

            auto mob = ev.mob();
            if (!mob.has_value()) {
                return;
            }

            auto& pos = mob->getPosition();

            logger->debug("[SpawnedMob] {}", pos.toString());

            auto land = db->getLandAt(pos, mob->getDimensionId());
            if (PreCheck(land)) {
                return;
            }

            if ((mob->hasCategory(::ActorCategory::Animal) && !land->getLandPermTableConst().allowAnimalSpawn)
                || (mob->hasCategory(::ActorCategory::Monster) && !land->getLandPermTableConst().allowMonsterSpawn)) {
                mob->despawn();
            }
        })
    };

    return true;
}


bool EventListener::release() {
    auto& bus = ll::event::EventBus::getInstance();

    for (auto& l : mListeners) {
        bus.removeListener(l);
    }

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
// std::unordered_set<string> EventListener::AnimalEntityMap = {
//     "minecraft:axolotl",          // 美西螈
//     "minecraft:bat",              // 蝙蝠
//     "minecraft:cat",              // 猫
//     "minecraft:chicken",          // 鸡
//     "minecraft:cod",              // 鳕鱼
//     "minecraft:cow",              // 牛
//     "minecraft:donkey",           // 驴
//     "minecraft:fox",              // 狐狸
//     "minecraft:glow_squid",       // 发光鱿鱼
//     "minecraft:horse",            // 马
//     "minecraft:mooshroom",        // 蘑菇牛
//     "minecraft:mule",             // 驴
//     "minecraft:ocelot",           // 豹猫
//     "minecraft:parrot",           // 鹦鹉
//     "minecraft:pig",              // 猪
//     "minecraft:rabbit",           // 兔子
//     "minecraft:salmon",           // 鲑鱼
//     "minecraft:snow_golem",       // 雪傀儡
//     "minecraft:sheep",            // 羊
//     "minecraft:skeleton_horse",   // 骷髅马
//     "minecraft:squid",            // 鱿鱼
//     "minecraft:strider",          // 炽足兽
//     "minecraft:tropical_fish",    // 热带鱼
//     "minecraft:turtle",           // 海龟
//     "minecraft:villager_v2",      // 村民
//     "minecraft:wandering_trader", // 流浪商人
//     "minecraft:npc"               // NPC
// };


} // namespace land
