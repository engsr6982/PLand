#include "InterceptorConfig.h"

#include "pland/PLand.h"
#include "pland/internal/interceptor/helper/EventTrace.h"
#include "pland/land/repo/LandContext.h"
#include "pland/reflect/TypeName.h"
#include "pland/utils/JsonUtil.h"

#include "ll/api/Config.h"
#include "ll/api/io/FileUtils.h"

#include "mc/world/item/VanillaItemNames.h"

#include <absl/container/flat_hash_map.h>

namespace land::internal::interceptor {


void InterceptorConfig::load(std::filesystem::path configDir) {
    auto path = configDir / FileName;
    if (!std::filesystem::exists(path) || !ll::config::loadConfig(cfg, path)) {
        save(configDir);
    }
    _buildDynamicRuleMap();
}

void InterceptorConfig::save(std::filesystem::path configDir) {
    auto path = configDir / FileName;
    ll::config::saveConfig(cfg, path);
}

absl::flat_hash_map<HashedString, RolePerms::Entry RolePerms::*, HashedStringHash, HashedStringEq> DynamicRuleMap = {};

void InterceptorConfig::_buildDynamicRuleMap() {
#define DECL_PERM_FIELD(T)                                                                                             \
    { reflect::getTemplateInnerLeafName<&T>(), &T }
    static absl::flat_hash_map<std::string_view, RolePerms::Entry RolePerms::*> const PermStr2MemberPointer = {
        DECL_PERM_FIELD(RolePerms::allowDestroy),
        DECL_PERM_FIELD(RolePerms::allowPlace),
        DECL_PERM_FIELD(RolePerms::useBucket),
        DECL_PERM_FIELD(RolePerms::useAxe),
        DECL_PERM_FIELD(RolePerms::useHoe),
        DECL_PERM_FIELD(RolePerms::useShovel),
        DECL_PERM_FIELD(RolePerms::placeBoat),
        DECL_PERM_FIELD(RolePerms::placeMinecart),
        DECL_PERM_FIELD(RolePerms::useButton),
        DECL_PERM_FIELD(RolePerms::useDoor),
        DECL_PERM_FIELD(RolePerms::useFenceGate),
        DECL_PERM_FIELD(RolePerms::allowInteractEntity),
        DECL_PERM_FIELD(RolePerms::useTrapdoor),
        DECL_PERM_FIELD(RolePerms::editSign),
        DECL_PERM_FIELD(RolePerms::useLever),
        DECL_PERM_FIELD(RolePerms::useFurnaces),
        DECL_PERM_FIELD(RolePerms::allowPlayerPickupItem),
        DECL_PERM_FIELD(RolePerms::allowRideTrans),
        DECL_PERM_FIELD(RolePerms::allowRideEntity),
        DECL_PERM_FIELD(RolePerms::usePressurePlate),
        DECL_PERM_FIELD(RolePerms::allowFishingRodAndHook),
        DECL_PERM_FIELD(RolePerms::allowUseThrowable),
        DECL_PERM_FIELD(RolePerms::useArmorStand),
        DECL_PERM_FIELD(RolePerms::allowDropItem),
        DECL_PERM_FIELD(RolePerms::useItemFrame),
        DECL_PERM_FIELD(RolePerms::useFlintAndSteel),
        DECL_PERM_FIELD(RolePerms::useBeacon),
        DECL_PERM_FIELD(RolePerms::useBed),
        DECL_PERM_FIELD(RolePerms::allowPvP),
        DECL_PERM_FIELD(RolePerms::allowHostileDamage),
        DECL_PERM_FIELD(RolePerms::allowFriendlyDamage),
        DECL_PERM_FIELD(RolePerms::allowSpecialEntityDamage),
        DECL_PERM_FIELD(RolePerms::useContainer),
        DECL_PERM_FIELD(RolePerms::useWorkstation),
        DECL_PERM_FIELD(RolePerms::useBell),
        DECL_PERM_FIELD(RolePerms::useCampfire),
        DECL_PERM_FIELD(RolePerms::useComposter),
        DECL_PERM_FIELD(RolePerms::useDaylightDetector),
        DECL_PERM_FIELD(RolePerms::useJukebox),
        DECL_PERM_FIELD(RolePerms::useNoteBlock),
        DECL_PERM_FIELD(RolePerms::useCake),
        DECL_PERM_FIELD(RolePerms::useComparator),
        DECL_PERM_FIELD(RolePerms::useRepeater),
        DECL_PERM_FIELD(RolePerms::useLectern),
        DECL_PERM_FIELD(RolePerms::useCauldron),
        DECL_PERM_FIELD(RolePerms::useRespawnAnchor),
        DECL_PERM_FIELD(RolePerms::useBoneMeal),
        DECL_PERM_FIELD(RolePerms::useBeeNest),
        DECL_PERM_FIELD(RolePerms::editFlowerPot),
        DECL_PERM_FIELD(RolePerms::allowUseRangedWeapon),
    };
    DynamicRuleMap.clear();

    auto& logger = PLand::getInstance().getSelf().getLogger();
    for (auto& [typeName, perm] : cfg.rules.item) {
        auto iter = PermStr2MemberPointer.find(perm);
        if (iter != PermStr2MemberPointer.end()) {
            DynamicRuleMap.try_emplace(HashedString{typeName}, iter->second);
        } else {
            logger.warn("Unknown item permission: {} ({}: {})", perm, typeName, perm);
        }
    }
    for (auto& [typeName, perm] : cfg.rules.block) {
        auto iter = PermStr2MemberPointer.find(perm);
        if (iter != PermStr2MemberPointer.end()) {
            DynamicRuleMap.try_emplace(HashedString{typeName}, iter->second);
        } else {
            logger.warn("Unknown block permission: {} ({}: {})", perm, typeName, perm);
        }
    }
}
RolePerms::Entry RolePerms::* InterceptorConfig::lookupDynamicRule(HashedString const& typeName) {
    TRACE_ADD_SCOPE("lookupDynamicRule");
    TRACE_LOG("lookup typename: {}", typeName.c_str());
    auto iter = DynamicRuleMap.find(typeName);
    if (iter != DynamicRuleMap.end()) {
#ifdef DEBUG
        if (cfg.rules.item.contains(typeName)) {
            TRACE_LOG(
                "In the {} table, find the permissions {} mapped to {}",
                "item",
                typeName.c_str(),
                cfg.rules.item[typeName]
            );
        }
        if (cfg.rules.block.contains(typeName)) {
            TRACE_LOG(
                "In the {} table, find the permissions {} mapped to {}",
                "block",
                typeName.c_str(),
                cfg.rules.block[typeName]
            );
        }
#endif
        return iter->second;
    }
    TRACE_LOG("Not found");
    return nullptr;
}

void InterceptorConfig::tryMigrate(std::filesystem::path configDir) {
    auto path = configDir / "Config.json";
    if (!std::filesystem::exists(path)) {
        return;
    }
    auto data = ll::file_utils::readFile(path);
    if (!data.has_value()) {
        return;
    }
    auto json = nlohmann::json::parse(*data);
    if (json.contains("version") && json["version"].get<int>() >= 32) {
        return;
    }
    ll::file_utils::writeFile(configDir / "Config.json.bak", *data);

    if (json.contains("listeners")) {
        auto& listeners                                  = json["listeners"];
        cfg.listeners.PlayerDestroyBlockEvent            = listeners["PlayerDestroyBlockEvent"].get<bool>();
        cfg.listeners.PlayerPlacingBlockEvent            = listeners["PlayerPlacingBlockEvent"].get<bool>();
        cfg.listeners.PlayerInteractBlockEvent           = listeners["PlayerInteractBlockEvent"].get<bool>();
        cfg.listeners.FireSpreadEvent                    = listeners["FireSpreadEvent"].get<bool>();
        cfg.listeners.PlayerAttackEvent                  = listeners["PlayerAttackEvent"].get<bool>();
        cfg.listeners.PlayerPickUpItemEvent              = listeners["PlayerPickUpItemEvent"].get<bool>();
        cfg.listeners.ArmorStandSwapItemBeforeEvent      = listeners["ArmorStandSwapItemBeforeEvent"].get<bool>();
        cfg.listeners.PlayerDropItemBeforeEvent          = listeners["PlayerDropItemBeforeEvent"].get<bool>();
        cfg.listeners.ActorRideBeforeEvent               = listeners["ActorRideBeforeEvent"].get<bool>();
        cfg.listeners.ExplosionBeforeEvent               = listeners["ExplosionBeforeEvent"].get<bool>();
        cfg.listeners.ActorHurtEvent                     = listeners["ActorHurtEvent"].get<bool>();
        cfg.listeners.MobHurtEffectBeforeEvent           = listeners["MobHurtEffectBeforeEvent"].get<bool>();
        cfg.listeners.PistonPushBeforeEvent              = listeners["PistonPushBeforeEvent"].get<bool>();
        cfg.listeners.PlayerOperatedItemFrameBeforeEvent = listeners["PlayerOperatedItemFrameBeforeEvent"].get<bool>();
        cfg.listeners.ActorTriggerPressurePlateBeforeEvent =
            listeners["ActorTriggerPressurePlateBeforeEvent"].get<bool>();
        cfg.listeners.RedstoneUpdateBeforeEvent         = listeners["RedstoneUpdateBeforeEvent"].get<bool>();
        cfg.listeners.WitherDestroyBeforeEvent          = listeners["WitherDestroyBeforeEvent"].get<bool>();
        cfg.listeners.MossGrowthBeforeEvent             = listeners["MossGrowthBeforeEvent"].get<bool>();
        cfg.listeners.LiquidFlowBeforeEvent             = listeners["LiquidFlowBeforeEvent"].get<bool>();
        cfg.listeners.SculkBlockGrowthBeforeEvent       = listeners["SculkBlockGrowthBeforeEvent"].get<bool>();
        cfg.listeners.SculkSpreadBeforeEvent            = listeners["SculkSpreadBeforeEvent"].get<bool>();
        cfg.listeners.PlayerEditSignBeforeEvent         = listeners["PlayerEditSignBeforeEvent"].get<bool>();
        cfg.listeners.SpawnedMobEvent                   = listeners["SpawnedMobEvent"].get<bool>();
        cfg.listeners.PlayerInteractEntityBeforeEvent   = listeners["PlayerInteractEntityBeforeEvent"].get<bool>();
        cfg.listeners.BlockFallBeforeEvent              = listeners["BlockFallBeforeEvent"].get<bool>();
        cfg.listeners.ActorDestroyBlockEvent            = listeners["ActorDestroyBlockEvent"].get<bool>();
        cfg.listeners.MobPlaceBlockBeforeEvent          = listeners["MobPlaceBlockBeforeEvent"].get<bool>();
        cfg.listeners.MobTakeBlockBeforeEvent           = listeners["MobTakeBlockBeforeEvent"].get<bool>();
        cfg.listeners.DragonEggBlockTeleportBeforeEvent = listeners["DragonEggBlockTeleportBeforeEvent"].get<bool>();
        json.erase("listeners");
    }
    if (json.contains("hooks")) {
        auto& hooks                       = json["hooks"];
        cfg.hooks.FishingHookHitHook      = hooks["registerFishingHookHitHook"].get<bool>();
        cfg.hooks.LayEggGoalHook          = hooks["registerLayEggGoalHook"].get<bool>();
        cfg.hooks.FireBlockBurnHook       = hooks["registerFireBlockBurnHook"].get<bool>();
        cfg.hooks.ChestBlockActorOpenHook = hooks["registerChestBlockActorOpenHook"].get<bool>();
        json.erase("hooks");
    }
    if (json.contains("protection")) {
        auto& protection = json["protection"];
        if (protection.contains("mob")) {
            auto& mob = protection["mob"];
            json_util::json2structWithDiffPatch(mob["hostileMobTypeNames"], cfg.rules.mob.allowHostileDamage);
            json_util::json2structWithDiffPatch(mob["passiveMobTypeNames"], cfg.rules.mob.allowFriendlyDamage);
            json_util::json2structWithDiffPatch(mob["specialMobTypeNames"], cfg.rules.mob.allowSpecialEntityDamage);
            json_util::json2structWithDiffPatch(
                mob["customSpecialMobTypeNames"],
                cfg.rules.mob.allowSpecialEntityDamage
            );
        }
        if (protection.contains("permissionMaps")) {
            auto& permissionMaps = protection["permissionMaps"];
            json_util::json2structWithDiffPatch(permissionMaps["itemSpecific"], cfg.rules.item);
            json_util::json2structWithDiffPatch(permissionMaps["blockSpecific"], cfg.rules.block);
            json_util::json2structWithDiffPatch(permissionMaps["blockFunctional"], cfg.rules.block);
        }
        json.erase("protection");
    }
    ll::file_utils::writeFile(path, json.dump(4));
}

decltype(InterceptorConfig::cfg) InterceptorConfig::cfg = [] {
    InterceptorConfig config;

    // std::string_view 显示转为 std::string, 避免 std::string_view::data 转换导致截止错误
    auto allowPlace           = std::string{reflect::getTemplateInnerLeafName<&RolePerms::allowPlace>()};
    auto useBoneMeal          = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useBoneMeal>()};
    auto allowUseRangedWeapon = std::string{reflect::getTemplateInnerLeafName<&RolePerms::allowUseRangedWeapon>()};
    config.rules.item         = {
        {           "minecraft:skull",           allowPlace}, // 头颅
        {          "minecraft:banner",           allowPlace}, // 旗帜
        {    "minecraft:glow_ink_sac",           allowPlace}, // 灯光染料
        {     "minecraft:end_crystal",           allowPlace}, // 末影水晶
        {       "minecraft:ender_eye",           allowPlace}, // 末影之眼
        {     "minecraft:armor_stand",           allowPlace}, // 盔甲架
        {       "minecraft:bone_meal",          useBoneMeal}, // 骨粉
        {     VanillaItemNames::Bow(), allowUseRangedWeapon}, // 弓
        {VanillaItemNames::Crossbow(), allowUseRangedWeapon}, // 弩
    };

    auto useContainer        = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useContainer>()};
    auto useCampfire         = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useCampfire>()};
    auto useComposter        = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useComposter>()};
    auto useNoteBlock        = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useNoteBlock>()};
    auto useJukebox          = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useJukebox>()};
    auto useBell             = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useBell>()};
    auto useDaylightDetector = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useDaylightDetector>()};
    auto useLectern          = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useLectern>()};
    auto useCauldron         = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useCauldron>()};
    auto useRespawnAnchor    = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useRespawnAnchor>()};
    auto editFlowerPot       = std::string{reflect::getTemplateInnerLeafName<&RolePerms::editFlowerPot>()};
    auto allowDestroy        = std::string{reflect::getTemplateInnerLeafName<&RolePerms::allowDestroy>()};
    auto useWorkstation      = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useWorkstation>()};
    auto useBeacon           = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useBeacon>()};
    auto useCake             = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useCake>()};
    auto useComparator       = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useComparator>()};
    auto useRepeater         = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useRepeater>()};
    auto useBeeNest          = std::string{reflect::getTemplateInnerLeafName<&RolePerms::useBeeNest>()};
    config.rules.block       = {
        /* 特殊功能与交互方块 */
        {                       "minecraft:chest",        useContainer}, // 箱子
        {               "minecraft:trapped_chest",        useContainer}, // 陷阱箱
        {                    "minecraft:campfire",         useCampfire}, // 篝火
        {               "minecraft:soul_campfire",         useCampfire}, // 灵魂篝火
        {                   "minecraft:composter",        useComposter}, // 堆肥桶
        {                   "minecraft:noteblock",        useNoteBlock}, // 音符盒
        {                     "minecraft:jukebox",          useJukebox}, // 唱片机
        {                        "minecraft:bell",             useBell}, // 钟
        {  "minecraft:daylight_detector_inverted", useDaylightDetector}, // 反转阳光探测器
        {           "minecraft:daylight_detector", useDaylightDetector}, // 阳光探测器
        {                     "minecraft:lectern",          useLectern}, // 讲台
        {                    "minecraft:cauldron",         useCauldron}, // 炼药锅
        {              "minecraft:respawn_anchor",    useRespawnAnchor}, // 重生锚
        {                  "minecraft:flower_pot",       editFlowerPot}, // 花盆
        {            "minecraft:sweet_berry_bush",        allowDestroy}, // 甜莓丛

        /* 功能类与工作台方块 */
        {           "minecraft:cartography_table",      useWorkstation}, // 制图台
        {              "minecraft:smithing_table",      useWorkstation}, // 锻造台
        {               "minecraft:brewing_stand",      useWorkstation}, // 酿造台
        {                       "minecraft:anvil",      useWorkstation}, // 铁砧
        {                  "minecraft:grindstone",      useWorkstation}, // 砂轮
        {            "minecraft:enchanting_table",      useWorkstation}, // 附魔台
        {                        "minecraft:loom",      useWorkstation}, // 织布机
        {           "minecraft:stonecutter_block",      useWorkstation}, // 切石机
        {                     "minecraft:crafter",      useWorkstation}, // 自动合成器
        {          "minecraft:chiseled_bookshelf",        useContainer}, // 雕纹书架
        {                      "minecraft:barrel",        useContainer}, // 木桶

        /* 铜箱子系列 */
        {                "minecraft:copper_chest",        useContainer}, // 铜箱子
        {        "minecraft:exposed_copper_chest",        useContainer}, // 斑驳的铜箱子
        {      "minecraft:weathered_copper_chest",        useContainer}, // 锈蚀的铜箱子
        {       "minecraft:oxidized_copper_chest",        useContainer}, // 氧化铜箱子
        {          "minecraft:waxed_copper_chest",        useContainer}, // 涂蜡铜箱子
        {  "minecraft:waxed_exposed_copper_chest",        useContainer}, // 涂蜡斑驳铜箱子
        {"minecraft:waxed_weathered_copper_chest",        useContainer}, // 涂蜡锈蚀铜箱子
        { "minecraft:waxed_oxidized_copper_chest",        useContainer}, // 涂蜡氧化铜箱子

        /* 容器与展示架类方块 */
        {               "minecraft:decorated_pot",        useContainer}, // 饰纹陶罐
        {                "minecraft:acacia_shelf",        useContainer}, // 金合欢木展示架/书架
        {                "minecraft:bamboo_shelf",        useContainer}, // 竹展示架/书架
        {                 "minecraft:birch_shelf",        useContainer}, // 白桦木展示架/书架
        {                "minecraft:cherry_shelf",        useContainer}, // 樱花木展示架/书架
        {               "minecraft:crimson_shelf",        useContainer}, // 绯红木展示架/书架
        {              "minecraft:dark_oak_shelf",        useContainer}, // 深色橡木展示架/书架
        {                "minecraft:jungle_shelf",        useContainer}, // 丛林木展示架/书架
        {              "minecraft:mangrove_shelf",        useContainer}, // 红树木展示架/书架
        {                   "minecraft:oak_shelf",        useContainer}, // 橡木展示架/书架
        {              "minecraft:pale_oak_shelf",        useContainer}, // 苍白橡木展示架/书架
        {                "minecraft:spruce_shelf",        useContainer}, // 云杉木展示架/书架
        {                "minecraft:warped_shelf",        useContainer}, // 诡异木展示架/书架

        /* 红石与存储容器 */
        {                      "minecraft:hopper",        useContainer}, // 漏斗
        {                     "minecraft:dropper",        useContainer}, // 投掷器
        {                   "minecraft:dispenser",        useContainer}, // 发射器
        {                       "minecraft:vault",        useContainer}, // 宝库
        {                      "minecraft:beacon",           useBeacon}, // 信标
        {                        "minecraft:cake",             useCake}, // 蛋糕
        {        "minecraft:unpowered_comparator",       useComparator}, // 红石比较器（未充能）
        {          "minecraft:powered_comparator",       useComparator}, // 红石比较器（已充能）
        {          "minecraft:unpowered_repeater",         useRepeater}, // 红石中继器（未充能）
        {            "minecraft:powered_repeater",         useRepeater}, // 红石中继器（已充能）
        {                    "minecraft:bee_nest",          useBeeNest}, // 蜂巢
        {                     "minecraft:beehive",          useBeeNest}, // 蜂箱
    };

    config.rules.mob.allowHostileDamage = {
        "minecraft:zombie",            // 僵尸
        "minecraft:skeleton",          // 骷髅
        "minecraft:creeper",           // 苦力怕
        "minecraft:spider",            // 蜘蛛
        "minecraft:enderman",          // 末影人
        "minecraft:witch",             // 女巫
        "minecraft:blaze",             // 烈焰人
        "minecraft:ghast",             // 恶魂
        "minecraft:magma_cube",        // 岩浆怪
        "minecraft:silverfish",        // 银鱼
        "minecraft:slime",             // 史莱姆
        "minecraft:guardian",          // 守卫者
        "minecraft:elder_guardian",    // 长老守卫者
        "minecraft:wither_skeleton",   // 凋零骷髅
        "minecraft:stray",             // 流浪者
        "minecraft:husk",              // 干尸
        "minecraft:zombie_villager",   // 僵尸村民
        "minecraft:drowned",           // 溺尸
        "minecraft:phantom",           // 幻翼
        "minecraft:pillager",          // 掠夺者
        "minecraft:vindicator",        // 卫道士
        "minecraft:ravager",           // 劫掠兽
        "minecraft:evocation_illager", // 唤魔者
        "minecraft:vex",               // 恼鬼
        "minecraft:shulker",           // 潜影贝
        "minecraft:endermite",         // 末影螨
        "minecraft:cave_spider",       // 洞穴蜘蛛
        "minecraft:zoglin",            // 僵尸疣猪兽
        "minecraft:piglin_brute",      // 猪灵残暴者
        "minecraft:hoglin",            // 疣猪兽
        "minecraft:wither",            // 凋零
        "minecraft:ender_dragon",      // 末影龙
        "minecraft:warden",            // 监守者
        "minecraft:piglin",            // 猪灵
        // --- 1.21.0 ~ 26.20  ---
        "minecraft:breeze",   // 旋风人
        "minecraft:bogged",   // 沼泽骷髅
        "minecraft:creaking", // 嘎吱怪
    };
    config.rules.mob.allowFriendlyDamage = {
        "minecraft:cow",       // 牛
        "minecraft:pig",       // 猪
        "minecraft:sheep",     // 羊
        "minecraft:chicken",   // 鸡
        "minecraft:rabbit",    // 兔子
        "minecraft:mooshroom", // 蘑菇牛
        "minecraft:horse",     // 马
        "minecraft:donkey",    // 驴
        "minecraft:mule",      // 骡
        "minecraft:ocelot",    // 猫
        "minecraft:bat",       // 蝙蝠
        "minecraft:sniffer",   // 探索者
        "minecraft:camel",     // 骆驼
        "minecraft:armadillo", // 犰狳
    };
    config.rules.mob.allowSpecialEntityDamage = {
        "minecraft:painting",               // 画
        "minecraft:hopper_minecart",        // 漏斗矿车
        "minecraft:chest_boat",             // 箱船
        "minecraft:leash_knot",             // 拴绳结
        "minecraft:armor_stand",            // 盔甲架
        "minecraft:minecart",               // 矿车
        "minecraft:command_block_minecart", // 指令方块矿车
        "minecraft:boat",                   // 船
        "minecraft:ender_crystal",          // 末影水晶
        "minecraft:chest_minecart",         // 运输矿车
        "minecraft:tnt_minecart",           // TNT矿车
    };
    return config;
}();


} // namespace land::internal::interceptor
