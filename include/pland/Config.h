#pragma once
#include "Global.h"
#include "ll/api/io/LogLevel.h"
#include <vector>


namespace land {

enum class EconomyKit : int { LegacyMoney, ScoreBoard };

struct Config {
    int              version{14};
    ll::io::LogLevel logLevel{ll::io::LogLevel::Info};

    struct {
        bool       enabled{true};                // 是否启用
        EconomyKit kit{EconomyKit::LegacyMoney}; // 经济系统
        string     economyName{"money"};         // 经济名称
        string     scoreboardObjName;            // 计分板对象名称
    } economy;

    struct {
        bool   landTp{true};    // 领地传送
        int    maxLand{20};     // 最大领地数量
        int    minSpacing{16};  // 最小领地间距
        double refundRate{0.9}; // 退款率(0.0~1.0，1.0为全额退款，0.9为退还90%)
        double discountRate{1}; // 折扣率(0.0~1.0，1.0为原价，0.9为打9折)

        bool setupDrawCommand{false}; // 安装领地绘制命令
        int  drawRange{64};           // 绘制 x 范围内的领地

        struct {
            bool   enabled{false};                              // 是否启用
            int    maxNested{5};                                // 最大嵌套层数(默认5，最大16)
            int    minSpacing{8};                               // 子领地之间的最小间距
            int    maxSubLand{6};                               // 每个领地的最大子领地数量
            string calculate{"(square * 8 + height * 20) * 0"}; // 价格公式
        } subLand;

        struct {
            bool enterTip{true};           // 进入领地提示
            bool bottomContinuedTip{true}; // 底部持续提示
            int  bottomTipFrequency{1};    // 底部提示频率(s)
        } tip;

        // 购买配置
        struct {
            struct {
                bool   enabled{true};
                string calculate{"square * 8 + height * 20"}; // 计算公式
            } threeDimensionl;

            struct {
                bool   enabled{true};
                string calculate{"square * 25"}; // 计算公式
            } twoDimensionl;

            struct {
                int min{4};       // 最小领地范围
                int max{60000};   // 最大领地范围
                int minHeight{1}; // 最小领地高度
            } squareRange;

            std::vector<LandDimid> allowDimensions{0, 1, 2}; // 允许的领地维度
        } bought;
    } land;

    struct {
        string tool{"minecraft:stick"}; // 工具
    } selector;

    struct {
        bool ActorHurtEvent{true};                            // 实体受伤事件
        bool PlayerDestroyBlockEvent{true};                   // 玩家破坏方块事件
        bool PlayerPlacingBlockEvent{true};                   // 玩家放置方块事件
        bool PlayerInteractBlockEvent{true};                  // 玩家交互方块事件(使用物品)
        bool FireSpreadEvent{true};                           // 火势蔓延事件
        bool PlayerAttackEvent{true};                         // 玩家攻击事件
        bool PlayerPickUpItemEvent{true};                     // 玩家拾取物品事件
        bool PlayerAttackBlockBeforeEvent{true};              // 玩家攻击方块事件
        bool ArmorStandSwapItemBeforeEvent{true};             // 盔甲架交换物品事件
        bool PlayerDropItemBeforeEvent{true};                 // 玩家丢弃物品事件
        bool ActorRideBeforeEvent{true};                      // 实体骑乘事件
        bool ExplosionBeforeEvent{true};                      // 爆炸事件
        bool FarmDecayBeforeEvent{true};                      // 农田枯萎事件
        bool MobHurtEffectBeforeEvent{true};                  // 生物受伤效果事件
        bool PistonPushBeforeEvent{true};                     // 活塞推动事件
        bool PlayerOperatedItemFrameBeforeEvent{true};        // 玩家操作物品展示框事件
        bool ActorTriggerPressurePlateBeforeEvent{true};      // 实体触发压力板事件
        bool ProjectileCreateBeforeEvent{true};               // 投掷物创建事件
        bool RedstoneUpdateBeforeEvent{true};                 // 红石更新事件
        bool WitherDestroyBeforeEvent{true};                  // 凋零破坏事件
        bool MossGrowthBeforeEvent{true};                     // 苔藓生长事件
        bool LiquidTryFlowBeforeEvent{true};                  // 液体尝试流动事件
        bool SculkBlockGrowthBeforeEvent{true};               // 诡秘方块生长事件
        bool SculkSpreadBeforeEvent{true};                    // 诡秘蔓延事件
        bool PlayerEditSignBeforeEvent{true};                 // 玩家编辑告示牌事件
        bool SpawnedMobEvent{true};                           // 生物生成事件(怪物和动物)
        bool SculkCatalystAbsorbExperienceBeforeEvent{false}; // 幽匿催化体吸收经验事件
        bool PlayerInteractEntityBeforeEvent{true}; // 实体交互事件
        bool BlockFallBeforeEvent{true};                 // 方块下落事件
        bool ActorDestroyBlockEvent{true}; // 实体破坏方块事件
        bool EndermanLeaveBlockEvent{true}; // 末影人搬走方块
        bool EndermanTakeBlockEvent{true};  // 末影人放下方块
        bool DragonEggBlockTeleportBeforeEvent{true}; // 龙蛋传送事件
    } listeners;

    struct {
        bool devTools{false}; // 开发工具
    } internal;

    // Functions
    LDAPI static Config cfg;
    LDAPI static bool   tryLoad();
    LDAPI static bool   trySave();
    LDAPI static bool   tryUpdate();
};


} // namespace land
