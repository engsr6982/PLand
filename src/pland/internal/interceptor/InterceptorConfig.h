#pragma once
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "pland/land/repo/LandContext.h"

#include <mc/deps/core/string/HashedString.h>

namespace land::internal::interceptor {

struct HashedStringHash {
    size_t operator()(HashedString const& value) const noexcept { return value.getHash(); }
};

struct HashedStringEq {
    bool operator()(HashedString const& lhs, HashedString const& rhs) const noexcept {
        return lhs.getHash() == rhs.getHash() && lhs.getString() == rhs.getString();
    }
};

struct InterceptorConfig {
    inline static constexpr int SchemaVersion = 3;

    int version = SchemaVersion;
    struct Listeners {
        bool PlayerDestroyBlockEvent              = true; // LL
        bool PlayerPlacingBlockEvent              = true; // LL
        bool PlayerInteractBlockEvent             = true; // LL
        bool PlayerAttackEvent                    = true; // LL
        bool PlayerPickUpItemEvent                = true; // LL
        bool SpawnedMobEvent                      = true; // LL (env)
        bool ActorHurtEvent                       = true; // LL
        bool FireSpreadEvent                      = true; // LL (env)
        bool ActorDestroyBlockEvent               = true; // ILA (env)
        bool MobTakeBlockBeforeEvent              = true; // ILA (env)
        bool MobPlaceBlockBeforeEvent             = true; // ILA (env)
        bool ActorPickupItemBeforeEvent           = true; // ILA (env)
        bool ActorRideBeforeEvent                 = true; // ILA
        bool MobHurtEffectBeforeEvent             = true; // ILA
        bool ActorTriggerPressurePlateBeforeEvent = true; // ILA
        bool PlayerInteractEntityBeforeEvent      = true; // ILA
        bool ArmorStandSwapItemBeforeEvent        = true; // ILA
        bool PlayerDropItemBeforeEvent            = true; // ILA
        bool PlayerOperatedItemFrameBeforeEvent   = true; // ILA
        bool PlayerEditSignBeforeEvent            = true; // ILA
        bool ExplosionBeforeEvent                 = true; // ILA (env)
        bool PistonPushBeforeEvent                = true; // ILA (env)
        bool RedstoneUpdateBeforeEvent            = true; // ILA (env)
        bool BlockFallBeforeEvent                 = true; // ILA (env)
        bool WitherDestroyBeforeEvent             = true; // ILA (env)
        bool MossGrowthBeforeEvent                = true; // ILA (env)
        bool LiquidFlowBeforeEvent                = true; // ILA (env)
        bool DragonEggBlockTeleportBeforeEvent    = true; // ILA (env)
        bool SculkBlockGrowthBeforeEvent          = true; // ILA (env)
        bool SculkSpreadBeforeEvent               = true; // ILA (env)
        bool PlayerUseItemEvent                   = true; // LL
    } listeners;

    struct Hooks {
        bool FishingHookHitHook{true};             // 钓鱼钩击中
        bool LayEggGoalHook{true};                 // 海龟产卵
        bool FireBlockBurnHook{true};              // 火焰蔓延
        bool ChestBlockActorOpenHook{true};        // 箱子打开
        bool LightningBoltHook{true};              // 闪电
        bool LecternBlockUseHook{true};            // 使用讲台
        bool LecternBlockDropBookHook{true};       // 取出讲台书本
        bool OozingMobEffectHook{true};            // 渗浆效果
        bool WeavingMobEffectHook{true};           // 盘丝效果
        bool HopperComponentPullInItemsHook{true}; // 漏斗组件吸取物品
        bool ExperienceOrbPlayerTouchHook{true};   // 拾取经验球
        bool ThrownTridentPlayerTouchHook{true};   // 拾取三叉戟
        bool ArrowPlayerTouchHook{true};           // 拾取箭
        bool AbstractArrowPlayerTouchHook{true};   // 拾取箭类投射物
        bool FarmChangeEventHook{true};            // 耕地踩踏/退化
        bool BigDripleafBlockHook{true};           // 大型垂滴叶
    } hooks;

    struct Rules {
        using Mapping = std::unordered_map<std::string, std::string>; // TypeName -> Permission
        struct Mob {
            using TypeNamesSet = std::unordered_set<HashedString, HashedStringHash, HashedStringEq>;
            TypeNamesSet allowHostileDamage;
            TypeNamesSet allowFriendlyDamage;
            TypeNamesSet allowSpecialEntityDamage;
        } mob;
        Mapping item;
        Mapping block;
    } rules;

    static InterceptorConfig cfg;

    inline static constexpr std::string_view FileName = "InterceptorConfig.json";

    static void load(std::filesystem::path configDir);
    static void save(std::filesystem::path configDir);

    static RolePerms::Entry RolePerms::* lookupDynamicRule(HashedString const& typeName);

    enum class MobRecordCategory : uint8_t {
        Undefined = 0,
        Hostile,
        Friendly,
        SpecialEntity,
    };
    // fast lookup table for mob dynamic rules (allowHostileDamage, allowFriendlyDamage, allowSpecialEntityDamage)
    // if mob is not found in the table, return MobRecordCategory::Undefined
    static MobRecordCategory lookupMobDynamicCategory(HashedString const& typeName);

    static void tryMigrateLegacyConfig(std::filesystem::path configDir);

private:
    static void _buildDynamicRuleMap();
};

static_assert(std::is_aggregate_v<InterceptorConfig>);

} // namespace land::internal::interceptor
