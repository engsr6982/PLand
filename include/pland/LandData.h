#pragma once
#include "ll/api/base/StdInt.h"
#include "nlohmann/json.hpp"
#include "pland/Global.h"
#include "pland/math/LandAABB.h"
#include <cstdint>
#include <vector>


namespace land {


struct LandPermTable {
    // 标记 [x] 为复用权限
    bool allowFireSpread{true};           // 火焰蔓延
    bool allowAttackDragonEgg{false};     // 点击龙蛋
    bool allowFarmDecay{true};            // 耕地退化
    bool allowPistonPushOnBoundary{true}; // 活塞推动
    bool allowRedstoneUpdate{true};       // 红石更新
    bool allowExplode{false};             // 爆炸
    bool allowBlockFall{false};           // 方块掉落
    bool allowDestroy{false};             // 允许破坏
    bool allowWitherDestroy{false};       // 允许凋零破坏
    bool allowPlace{false};               // 允许放置 [x]
    bool allowPlayerDamage{false};        // 允许玩家受伤
    bool allowMonsterDamage{true};        // 允许敌对生物受伤
    bool allowPassiveDamage{false};       // 允许友好、中立生物受伤
    bool allowSpecialDamage{false};       // 允许对特殊实体造成伤害(船、矿车、画等)
    bool allowSpecialDamage2{false};       // 允许对特殊实体2造成伤害
    bool allowOpenChest{false};           // 允许打开箱子
    bool allowPickupItem{false};          // 允许拾取物品
    bool allowEndermanLeaveBlock{false};  // 允许末影人放下方块

    bool allowDropItem{true};            // 允许丢弃物品
    bool allowProjectileCreate{false};   // 允许投掷物
    bool allowRideEntity{false};         // 允许骑乘实体
    bool allowRideTrans{false};          // 允许骑乘矿车、船
    bool allowAxePeeled{false};          // 允许斧头去皮
    bool allowAttackEnderCrystal{false}; // 允许攻击末地水晶
    bool allowDestroyArmorStand{false};  // 允许破坏盔甲架
    bool allowLiquidFlow{true};          // 允许液体流动
    bool allowSculkBlockGrowth{true};    // 允许幽匿尖啸体生长
    bool allowMonsterSpawn{true};        // 允许怪物生成
    bool allowAnimalSpawn{true};         // 允许动物生成
    bool allowInteractEntity{false};     // 实体交互
    bool allowActorDestroy{false};       // 实体破坏
    bool allowAttackPainting{false};     // 攻击画
    bool allowAttackMinecart{false};     // 攻击矿车
    bool allowAttackBoat{false};         // 攻击船

    bool useAnvil{false};             // 使用铁砧
    bool useBarrel{false};            // 使用木桶
    bool useBeacon{false};            // 使用信标
    bool useBed{false};               // 使用床
    bool useBell{false};              // 使用钟
    bool useBlastFurnace{false};      // 使用高炉
    bool useBrewingStand{false};      // 使用酿造台
    bool useCampfire{false};          // 使用营火
    bool useFlintAndSteel{false};     // 使用打火石
    bool useCartographyTable{false};  // 使用制图台
    bool useComposter{false};         // 使用堆肥桶
    bool useCraftingTable{false};     // 使用工作台
    bool useDaylightDetector{false};  // 使用阳光探测器
    bool useDispenser{false};         // 使用发射器
    bool useDropper{false};           // 使用投掷器
    bool useEnchantingTable{false};   // 使用附魔台
    bool useDoor{false};              // 使用门
    bool useFenceGate{false};         // 使用栅栏门
    bool useFurnace{false};           // 使用熔炉
    bool useGrindstone{false};        // 使用砂轮
    bool useHopper{false};            // 使用漏斗
    bool useJukebox{false};           // 使用唱片机
    bool useLoom{false};              // 使用织布机
    bool useStonecutter{false};       // 使用切石机
    bool useNoteBlock{false};         // 使用音符盒
    bool useCrafter{false};           // 使用合成器
    bool useChiseledBookshelf{false}; // 使用雕纹书架
    bool useCake{false};              // 吃蛋糕
    bool useComparator{false};        // 使用红石比较器
    bool useRepeater{false};          // 使用红石中继器
    bool useShulkerBox{false};        // 使用潜影盒
    bool useSmithingTable{false};     // 使用锻造台
    bool useSmoker{false};            // 使用烟熏炉
    bool useTrapdoor{false};          // 使用活板门
    bool useLectern{false};           // 使用讲台
    bool useCauldron{false};          // 使用炼药锅
    bool useLever{false};             // 使用拉杆
    bool useButton{false};            // 使用按钮
    bool useRespawnAnchor{false};     // 使用重生锚
    bool useItemFrame{false};         // 使用物品展示框
    bool useFishingHook{false};       // 使用钓鱼竿
    bool useBucket{false};            // 使用桶
    bool usePressurePlate{false};     // 使用压力板
    bool useArmorStand{false};        // 使用盔甲架
    bool useBoneMeal{false};          // 使用骨粉
    bool useHoe{false};               // 使用锄头
    bool useShovel{false};            // 使用锹
    bool useVault{false};             // 使用试炼宝库
    bool useBeeNest{false};           // 使用蜂巢蜂箱
    bool placeBoat{false};            // 放置船
    bool placeMinecart{false};        // 放置矿车

    bool editFlowerPot{false}; // 编辑花盆
    bool editSign{false};      // 编辑告示牌
};


using LandData_sptr           = std::shared_ptr<class LandData>; // 共享指针
using LandData_wptr           = std::weak_ptr<class LandData>;   // 弱指针
constexpr int LandDataVersion = 18;                              // 领地数据版本号
class LandData {
public:
    int                 version{LandDataVersion};              // 版本号
    LandAABB            mPos;                                  // 领地对角坐标
    LandPos             mTeleportPos;                          // 领地传送坐标
    LandID              mLandID{LandID(-1)};                   // 领地唯一ID  (由 PLand::addLand() 时分配)
    LandDimid           mLandDimid;                            // 领地所在维度
    bool                mIs3DLand;                             // 是否为3D领地
    LandPermTable       mLandPermTable;                        // 领地权限
    UUIDs               mLandOwner;                            // 领地主人(默认UUID,其余情况看mOwnerDataIsXUID)
    std::vector<UUIDs>  mLandMembers;                          // 领地成员
    std::string         mLandName{"Unnamed territories"_tr()}; // 领地名称
    std::string         mLandDescribe{"No description"_tr()};  // 领地描述
    bool                mIsSaleing{false};                     // 是否正在出售
    int                 mSalePrice{0};                         // 出售价格
    int                 mOriginalBuyPrice{0};                  // 原始购买价格
    bool                mIsConvertedLand{false};               // 是否为转换后的领地(其它插件创建的领地)
    bool                mOwnerDataIsXUID{false};   // 领地主人数据是否为XUID (如果为true，则主人上线自动转换为UUID)
    LandID              mParentLandID{LandID(-1)}; // 父领地ID
    std::vector<LandID> mSubLandIDs;               // 子领地ID


public:
    LDNDAPI static LandData_sptr make(); // 创建一个空领地数据(反射使用)
    LDNDAPI static LandData_sptr
    make(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner); // 新建领地数据

    // getters
    LDNDAPI LandAABB const& getLandPos() const;
    LDNDAPI LandID          getLandID() const;
    LDNDAPI LandDimid       getLandDimid() const;
    LDNDAPI int             getSalePrice() const;

    LDNDAPI LandPermTable&       getLandPermTable();
    LDNDAPI LandPermTable const& getLandPermTable() const;
    LDNDAPI LandPermTable const& getLandPermTableConst() const;

    LDNDAPI UUIDs const& getLandOwner() const;
    LDNDAPI std::vector<UUIDs> const& getLandMembers() const;
    LDNDAPI std::string const& getLandName() const;
    LDNDAPI std::string const& getLandDescribe() const;


    // setters
    LDAPI bool setSaleing(bool isSaleing);
    LDAPI bool setIs3DLand(bool is3D);
    LDAPI bool setLandOwner(UUIDs const& uuid);
    LDAPI bool setSalePrice(int price);

    LDAPI bool setLandName(std::string const& name);
    LDAPI bool setLandDescribe(std::string const& describe);
    LDAPI bool _setLandPos(LandAABB const& pos); // private

    LDAPI bool addLandMember(UUIDs const& uuid);
    LDAPI bool removeLandMember(UUIDs const& uuid);


    // others
    LDNDAPI bool is3DLand() const;
    LDNDAPI bool isLandOwner(UUIDs const& uuid) const;
    LDNDAPI bool isLandMember(UUIDs const& uuid) const;
    LDNDAPI bool isSaleing() const;

    /**
     * @brief 是否有父领地
     */
    LDNDAPI bool hasParentLand() const;

    /**
     * @brief 是否有子领地
     */
    LDNDAPI bool hasSubLand() const;

    /**
     * @brief 是否为子领地(有父领地、无子领地)
     */
    LDNDAPI bool isSubLand() const;

    /**
     * @brief 是否为父领地(有子领地、无父领地)
     */
    LDNDAPI bool isParentLand() const;

    /**
     * @brief 是否为混合领地(有父领地、有子领地)
     */
    LDNDAPI bool isMixLand() const;

    /**
     * @brief 是否为普通领地(无父领地、无子领地)
     */
    LDNDAPI bool isOrdinaryLand() const;

    /**
     * @brief 是否可以创建子领地
     * 如果满足嵌套层级限制，则可以创建子领地
     */
    LDNDAPI bool canCreateSubLand() const;

    /**
     * @brief 获取父领地
     */
    LDNDAPI LandData_sptr getParentLand() const;

    /**
     * @brief 获取子领地
     */
    LDNDAPI std::vector<LandData_sptr> getSubLands() const;

    /**
     * @brief 获取嵌套层级(相对于父领地)
     */
    LDNDAPI int getNestedLevel() const;

    /**
     * @brief 获取根领地(即最顶层的普通领地 isOrdinaryLand() == true)
     */
    LDNDAPI LandData_sptr getRootLand() const;

    LDNDAPI bool isRadiusInLand(BlockPos const& pos, int radius) const;
    LDNDAPI bool isAABBInLand(BlockPos const& pos1, BlockPos const& pos2) const;

    LDNDAPI LandPermType getPermType(UUIDs const& uuid) const;

    LDAPI void load(nlohmann::json& json);
    LDNDAPI nlohmann::json toJSON() const;

    LDNDAPI bool operator==(LandData_sptr const& other) const;
};


} // namespace land