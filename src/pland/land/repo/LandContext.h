#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/enums/LandHoldType.h"
#include "pland/enums/LeaseState.h"

#include <vector>


namespace land {

struct EnvironmentPerms final {
    bool allowFireSpread{true};              // 火焰蔓延
    bool allowMonsterSpawn{true};            // 怪物生成
    bool allowAnimalSpawn{true};             // 动物生成
    bool allowMobGrief{false};               // 实体破坏(破坏/拾取/放置方块) v25 allowActorDestroy=false, enderman=false
    bool allowExplode{false};                // 爆炸
    bool allowFarmDecay{true};               // 耕地退化
    bool allowPistonPushOnBoundary{true};    // 活塞推动边界方块
    bool allowRedstoneUpdate{true};          // 红石更新
    bool allowBlockFall{false};              // 方块掉落
    bool allowWitherDestroy{false};          // 凋零破坏
    bool allowMossGrowth{true};              // 苔藓生长(蔓延) v27
    bool allowLiquidFlow{true};              // 流动液体
    bool allowDragonEggTeleport{false};      // 龙蛋传送 v25 allowAttackDragonEgg=false
    bool allowSculkBlockGrowth{true};        // 幽匿尖啸体生长
    bool allowSculkSpread{false};            // 幽匿蔓延 v27
    bool allowLightningBolt{true};           // 闪电
    bool allowMinecartHopperPullItems{true}; // 矿车漏斗拉取物品
};
struct RolePerms final {
    struct Entry final {
        bool member;
        bool actor;
    };
    Entry allowDestroy{true, false};        // 允许破坏方块
    Entry allowPlace{true, false};          // 允许放置方块
    Entry useBucket{true, false};           // 允许使用桶(水/岩浆/...)
    Entry useAxe{true, false};              // 允许使用斧头
    Entry useHoe{true, false};              // 允许使用锄头
    Entry useShovel{true, false};           // 允许使用铲子
    Entry placeBoat{true, false};           // 允许放置船
    Entry placeMinecart{true, false};       // 允许放置矿车
    Entry useButton{true, false};           // 允许使用按钮
    Entry useDoor{true, false};             // 允许使用门
    Entry useFenceGate{true, false};        // 允许使用栅栏门
    Entry allowInteractEntity{true, false}; // 允许与实体交互 // TODO: 解决歧义：玩家交互实体 & 玩家取走栅栏上的拴绳实体
    Entry useTrapdoor{true, false};         // 允许使用活板门
    Entry editSign{true, false};            // 允许编辑告示牌
    Entry useLever{true, false};            // 允许使用拉杆
    Entry useFurnaces{true, false};         // 允许使用所有熔炉类方块（熔炉/高炉/烟熏炉）
    Entry allowPlayerPickupItem{true, false};  // 允许玩家拾取物品
    Entry allowRideTrans{true, false};         // 允许骑乘运输工具（矿车/船）
    Entry allowRideEntity{true, false};        // 允许骑乘实体
    Entry usePressurePlate{true, false};       // 触发压力板
    Entry allowFishingRodAndHook{true, false}; // 允许使用钓鱼竿和鱼钩
    Entry allowUseThrowable{true, false};      // 允许使用投掷物(雪球/鸡蛋/三叉戟/...)
    Entry useArmorStand{true, false};          // 允许使用盔甲架
    Entry allowDropItem{true, true};           // 允许丢弃物品
    Entry useItemFrame{true, false};           // 允许操作物品展示框
    Entry useFlintAndSteel{true, false};       // 使用打火石
    Entry useBeacon{true, false};              // 使用信标
    Entry useBed{true, false};                 // 使用床

    // 以下权限均通过 PermMapping 动态映射
    Entry allowPvP{false, false};                 // 允许PvP v25 allowPlayerDamage=false
    Entry allowHostileDamage{true, true};         // 敌对生物受到伤害 v25 allowMonsterDamage=true
    Entry allowFriendlyDamage{false, false};      // 友好生物受到伤害
    Entry allowSpecialEntityDamage{false, false}; // 特殊生物受到伤害
    Entry useContainer{true, false};   // 允许使用容器(箱子/木桶/潜影盒/发射器/投掷器/漏斗/雕纹书架/试炼宝库/...)
    Entry useWorkstation{true, false}; // 工作站类(工作台/铁砧/附魔台/酿造台/锻造台/砂轮/织布机/切石机/制图台/合成器)
    Entry useBell{true, false};        // 使用钟
    Entry useCampfire{true, false};    // 使用营火
    Entry useComposter{true, false};   // 使用堆肥桶
    Entry useDaylightDetector{true, false};  // 使用阳光探测器
    Entry useJukebox{true, false};           // 使用唱片机
    Entry useNoteBlock{true, false};         // 使用音符盒
    Entry useCake{true, false};              // 吃蛋糕
    Entry useComparator{true, false};        // 使用红石比较器
    Entry useRepeater{true, false};          // 使用红石中继器
    Entry useLectern{true, false};           // 使用讲台
    Entry useCauldron{true, false};          // 使用炼药锅
    Entry useRespawnAnchor{true, false};     // 使用重生锚
    Entry useBoneMeal{true, false};          // 使用骨粉
    Entry useBeeNest{true, false};           // 使用蜂巢(蜂箱)
    Entry editFlowerPot{true, false};        // 编辑花盆
    Entry allowUseRangedWeapon{true, false}; // 允许使用远程武器(弓/弩)

    // 新增权限(保持 ABI 稳定，不在前面插入字段，避免 LRCA 需要重新编译)
    Entry allowTriggerDripleaf{true, true}; // 允许触发垂滴叶
};
struct LandPermTable final {
    EnvironmentPerms environment{};
    RolePerms        role{};
};

// ! 注意：如果 LandContext 有更改，则必须递增 LandSchemaVersion，否则导致加载异常
// 对于字段变动、重命名，请注册对应的 migrator 转换数据
constexpr int LandSchemaVersion = 31;
struct LandContext {
    int                      version{LandSchemaVersion};            // 版本号
    LandAABB                 mPos{};                                // 领地对角坐标
    LandPos                  mTeleportPos{};                        // 领地传送坐标
    LandID                   mLandID{INVALID_LAND_ID};              // 领地唯一ID  (由 LandRegistry::addLand() 时分配)
    LandDimid                mLandDimid{};                          // 领地所在维度
    bool                     mIs3DLand{};                           // 是否为3D领地
    LandPermTable            mLandPermTable{};                      // 领地权限
    std::string              mLandOwner{};                          // 领地主人(默认UUID,其余情况看mOwnerDataIsXUID)
    std::vector<std::string> mLandMembers{};                        // 领地成员
    std::string              mLandName{"Unnamed territories"};      // 领地名称
    int                      mOriginalBuyPrice{0};                  // 原始购买价格
    LandHoldType             mHoldType{LandHoldType::Bought};       // 购买/租赁模式
    struct LeaseInfo {
        LeaseState mState{LeaseState::None}; // 租赁状态
        time_t     mStartAt{0};              // 租赁开始时间(秒)
        time_t     mEndAt{0};                // 租赁到期时间(秒)
    } mLeasing;
    [[deprecated]] bool mIsConvertedLand{false};        // 是否为转换后的领地(其它插件创建的领地)
    [[deprecated]] bool mOwnerDataIsXUID{false};        // 领地主人数据是否为XUID (如果为true，则主人上线自动转换为UUID)
    LandID              mParentLandID{INVALID_LAND_ID}; // 父领地ID
    std::vector<LandID> mSubLandIDs{};                  // 子领地ID
};

STATIC_ASSERT_AGGREGATE(LandPermTable);
STATIC_ASSERT_AGGREGATE(LandContext);
template <typename T, typename I>
concept AssertPosField = requires(T const& t, I const& i) {
    { t.min } -> std::convertible_to<I>;
    { t.max } -> std::convertible_to<I>;
    { i.x } -> std::convertible_to<int>;
    { i.y } -> std::convertible_to<int>;
    { i.z } -> std::convertible_to<int>;
};
static_assert(
    AssertPosField<LandAABB, LandPos>,
    "If the field is changed, please update the data transformation rules accordingly."
);

} // namespace land
