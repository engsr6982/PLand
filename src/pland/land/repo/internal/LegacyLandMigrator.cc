#include "LegacyLandMigrator.h"

#include <nlohmann/json.hpp>

namespace land {
namespace internal {

LegacyLandMigrator& LegacyLandMigrator::getInstance() {
    static LegacyLandMigrator instance;
    return instance;
}

LegacyLandMigrator::LegacyLandMigrator() {
    registerMigrationUnit(15, [](LegacyLandMigrator::container_t& data) -> bool {
        static constexpr std::string_view LegacyMaxKey = "mMax_B";
        static constexpr std::string_view LegacyMinKey = "mMin_A";
        static constexpr std::string_view NewMaxKey    = "max";
        static constexpr std::string_view NewMinKey    = "min";

        auto& pos = data.at("mPos");
        if (pos.contains(LegacyMaxKey)) {
            auto legacyMax = pos[LegacyMaxKey]; // copy
            pos.erase(LegacyMaxKey);
            pos[NewMaxKey] = std::move(legacyMax);
        }
        if (pos.contains(LegacyMinKey)) {
            auto legacyMin = pos[LegacyMinKey]; // copy
            pos.erase(LegacyMinKey);
            pos[NewMinKey] = std::move(legacyMin);
        }
        return true;
    });

    // 25/26 -> 27
    registerMigrationUnit(27, [](LegacyLandMigrator::container_t& data) -> bool {
        if (!data.contains("mLandPermTable")) return true;

        // 1. 读取并备份旧权限数据
        auto oldPerms = data["mLandPermTable"]; // copy
        if (!oldPerms.is_object()) return true; // 防御性检查

        // 2. 准备新的分层结构
        LegacyLandMigrator::container_t newEnv  = LegacyLandMigrator::container_t::object();
        LegacyLandMigrator::container_t newRole = LegacyLandMigrator::container_t::object();

        // --- 辅助 Lambda ---

        // 获取 v25 布尔值，如果不存在则使用默认值 (对应 v25 结构体默认值)
        auto getV25 = [&](const std::string& key, bool defaultVal) -> bool {
            if (oldPerms.contains(key) && oldPerms[key].is_boolean()) {
                return oldPerms[key].get<bool>();
            }
            return defaultVal;
        };

        // 创建角色权限 Entry (member, guest)
        // isPrivilege: true 表示这是特权(如破坏/开箱)，成员默认为 true；
        //              false 表示这是规则(如PvP/火焰)，成员跟随 v25 设置
        auto makeEntry = [&](bool v25Val, bool isPrivilege) -> LegacyLandMigrator::container_t {
            return {
                { "guest",                      v25Val},
                {"member", isPrivilege ? true : v25Val}
                // 逻辑：如果是 v25=true (公开)，则 member=true, guest=true
                //      如果是 v25=false (私有)，且 isPrivilege=true，则 member=true (成员特权), guest=false
                //      如果是 v25=false (禁止PvP)，且 isPrivilege=false，则 member=false, guest=false
            };
        };

        // --- 环境权限迁移 (EnvironmentPerms) ---
        // 映射规则：直接对应或合并
        newEnv["allowFireSpread"]           = getV25("allowFireSpread", true);
        newEnv["allowMonsterSpawn"]         = getV25("allowMonsterSpawn", true);
        newEnv["allowAnimalSpawn"]          = getV25("allowAnimalSpawn", true);
        newEnv["allowExplode"]              = getV25("allowExplode", false);
        newEnv["allowFarmDecay"]            = getV25("allowFarmDecay", true);
        newEnv["allowPistonPushOnBoundary"] = getV25("allowPistonPushOnBoundary", true);
        newEnv["allowRedstoneUpdate"]       = getV25("allowRedstoneUpdate", true);
        newEnv["allowBlockFall"]            = getV25("allowBlockFall", false);
        newEnv["allowWitherDestroy"]        = getV25("allowWitherDestroy", false);
        newEnv["allowLiquidFlow"]           = getV25("allowLiquidFlow", true);
        newEnv["allowSculkBlockGrowth"]     = getV25("allowSculkBlockGrowth", true);

        // 重命名/合并项
        // 龙蛋传送 <- 点击龙蛋
        newEnv["allowDragonEggTeleport"] = getV25("allowAttackDragonEgg", false);

        // 实体破坏 <- 实体破坏(v25) AND 末影人搬方块(v25)
        // 冲突处理：任意一个为 false 则结果为 false (更严格的保护)
        bool allowActorDestroy  = getV25("allowActorDestroy", false);
        bool allowEnderman      = getV25("allowEndermanLeaveBlock", false);
        newEnv["allowMobGrief"] = allowActorDestroy && allowEnderman;

        // --- 角色权限迁移 (RolePerms) ---
        // 参数2 (isPrivilege): true=成员默认有权限, false=成员跟随旧值

        // 基础交互
        newRole["allowDestroy"]          = makeEntry(getV25("allowDestroy", false), true);
        newRole["allowPlace"]            = makeEntry(getV25("allowPlace", false), true);
        newRole["useBucket"]             = makeEntry(getV25("useBucket", false), true);
        newRole["useAxe"]                = makeEntry(getV25("allowAxePeeled", false), true); // 重命名
        newRole["useHoe"]                = makeEntry(getV25("useHoe", false), true);
        newRole["useShovel"]             = makeEntry(getV25("useShovel", false), true);
        newRole["placeBoat"]             = makeEntry(getV25("placeBoat", false), true);
        newRole["placeMinecart"]         = makeEntry(getV25("placeMinecart", false), true);
        newRole["useButton"]             = makeEntry(getV25("useButton", false), true);
        newRole["useDoor"]               = makeEntry(getV25("useDoor", false), true);
        newRole["useFenceGate"]          = makeEntry(getV25("useFenceGate", false), true);
        newRole["allowInteractEntity"]   = makeEntry(getV25("allowInteractEntity", false), true);
        newRole["useTrapdoor"]           = makeEntry(getV25("useTrapdoor", false), true);
        newRole["editSign"]              = makeEntry(getV25("editSign", false), true);
        newRole["useLever"]              = makeEntry(getV25("useLever", false), true);
        newRole["allowPlayerPickupItem"] = makeEntry(getV25("allowPickupItem", false), true); // 重命名
        newRole["allowRideTrans"]        = makeEntry(getV25("allowRideTrans", false), true);
        newRole["allowRideEntity"]       = makeEntry(getV25("allowRideEntity", false), true);
        newRole["usePressurePlate"]      = makeEntry(getV25("usePressurePlate", false), true);
        newRole["allowFishingRodAndHook"] =
            makeEntry(getV25("allowFishingRodAndHook", false), false); // 钓鱼钩可能有破坏性，视作规则
        newRole["allowUseThrowable"] = makeEntry(getV25("allowProjectileCreate", false), false); // 弹射物视作规则
        newRole["useArmorStand"]     = makeEntry(getV25("useArmorStand", false), true);
        newRole["allowDropItem"]     = makeEntry(getV25("allowDropItem", true), true);
        newRole["useItemFrame"]      = makeEntry(getV25("useItemFrame", false), true);
        newRole["useFlintAndSteel"]  = makeEntry(getV25("useFlintAndSteel", false), true);
        newRole["useBeacon"]         = makeEntry(getV25("useBeacon", false), true);
        newRole["useBed"]            = makeEntry(getV25("useBed", false), true);

        // 杂项单点映射
        newRole["useBell"]             = makeEntry(getV25("useBell", false), true);
        newRole["useCampfire"]         = makeEntry(getV25("useCampfire", false), true);
        newRole["useComposter"]        = makeEntry(getV25("useComposter", false), true);
        newRole["useDaylightDetector"] = makeEntry(getV25("useDaylightDetector", false), true);
        newRole["useJukebox"]          = makeEntry(getV25("useJukebox", false), true);
        newRole["useNoteBlock"]        = makeEntry(getV25("useNoteBlock", false), true);
        newRole["useCake"]             = makeEntry(getV25("useCake", false), true);
        newRole["useComparator"]       = makeEntry(getV25("useComparator", false), true);
        newRole["useRepeater"]         = makeEntry(getV25("useRepeater", false), true);
        newRole["useLectern"]          = makeEntry(getV25("useLectern", false), true);
        newRole["useCauldron"]         = makeEntry(getV25("useCauldron", false), true);
        newRole["useRespawnAnchor"]    = makeEntry(getV25("useRespawnAnchor", false), true);
        newRole["useBoneMeal"]         = makeEntry(getV25("useBoneMeal", false), true);
        newRole["useBeeNest"]          = makeEntry(getV25("useBeeNest", false), true);
        newRole["editFlowerPot"]       = makeEntry(getV25("editFlowerPot", false), true);

        // --- 复杂合并项 (Conflicts resolve to False) ---

        // 1. PvP / 伤害
        // v25 allowPlayerDamage=false (默认不允许PvP)。如果 v25=false，则成员和访客都应为 false。
        newRole["allowPvP"]            = makeEntry(getV25("allowPlayerDamage", false), false);
        newRole["allowHostileDamage"]  = makeEntry(getV25("allowMonsterDamage", true), false); // 默认允许打怪
        newRole["allowFriendlyDamage"] = makeEntry(getV25("allowPassiveDamage", false), false);

        // 特殊伤害合并：任意一个 false 则 false
        bool special1                       = getV25("allowSpecialDamage", false);
        bool special2                       = getV25("allowCustomSpecialDamage", false);
        newRole["allowSpecialEntityDamage"] = makeEntry(special1 && special2, false);

        // 2. 容器 (useContainer)
        // 合并：Chest, Barrel, Shulker, Dispenser, Dropper, Hopper, ChiseledBookshelf, Vault
        bool canOpenChest = getV25("allowOpenChest", false); // v25 主键
        bool canBarrel    = getV25("useBarrel", false);
        bool canShulker   = getV25("useShulkerBox", false);
        bool canDispenser = getV25("useDispenser", false);
        bool canDropper   = getV25("useDropper", false);
        bool canHopper    = getV25("useHopper", false);
        bool canBookshelf = getV25("useChiseledBookshelf", false);
        bool canVault     = getV25("useVault", false);

        // 严格执行：冲突使用 false (即必须全部为 true 才能开启 v27 的 useContainer)
        // 风险提示：如果玩家以前只开了箱子没开桶，迁移后箱子也会被锁住！
        bool mergedContainer = canOpenChest && canBarrel && canShulker && canDispenser && canDropper && canHopper
                            && canBookshelf && canVault;
        newRole["useContainer"] = makeEntry(mergedContainer, true);

        // 3. 熔炉类 (useFurnaces)
        bool canFurnace        = getV25("useFurnace", false);
        bool canBlast          = getV25("useBlastFurnace", false);
        bool canSmoker         = getV25("useSmoker", false);
        newRole["useFurnaces"] = makeEntry(canFurnace && canBlast && canSmoker, true);

        // 4. 工作站类 (useWorkstation)
        // 合并大量方块
        bool w1  = getV25("useCraftingTable", false);
        bool w2  = getV25("useAnvil", false);
        bool w3  = getV25("useEnchantingTable", false);
        bool w4  = getV25("useBrewingStand", false);
        bool w5  = getV25("useSmithingTable", false);
        bool w6  = getV25("useGrindstone", false);
        bool w7  = getV25("useLoom", false);
        bool w8  = getV25("useStonecutter", false);
        bool w9  = getV25("useCartographyTable", false);
        bool w10 = getV25("useCrafter", false);

        bool mergedWorkstation    = w1 && w2 && w3 && w4 && w5 && w6 && w7 && w8 && w9 && w10;
        newRole["useWorkstation"] = makeEntry(mergedWorkstation, true);

        // 3. 应用变更
        data["mLandPermTable"].clear();
        data["mLandPermTable"]["environment"] = newEnv;
        data["mLandPermTable"]["role"]        = newRole;

        return true;
    });

    registerMigrationUnit(31, [](LegacyLandMigrator::container_t& root) -> bool {
        if (!root.contains("mLandPermTable")) {
            return true;
        }
        auto& mLandPermTable = root["mLandPermTable"];
        auto& role           = mLandPermTable["role"];

        static constexpr auto legacyField = "guest";
        static constexpr auto newField    = "actor";

        for (auto& [key, entry] : role.items()) {
            mapPath(role, fmt::format("{}.{}", key, legacyField), fmt::format("{}.{}", key, newField));
        }
        return true;
    });
}

} // namespace internal
} // namespace land