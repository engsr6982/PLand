# 配置文件

> 配置文件为`json`格式，位于`plugins/PLand/`目录下，文件名为`config.json`。

!> 配置文件为`json`格式，请勿使用记事本等不支持`json`格式的文本编辑器进行编辑，否则会导致配置文件损坏。

## 配置文件结构

```json
{
  "version": 15, // 配置文件版本，请勿修改
  "logLevel": "Info", // 日志等级 Off / Fatal / Error / Warn / Info / Debug / Trace
  "economy": {
    "enabled": true, // 是否启用经济系统
    "kit": "LegacyMoney", // 经济套件 LegacyMoney 或 ScoreBoard
    "economyName": "money", // 经济名称
    "scoreboardObjName": "" // 计分板对象名称
  },
  "land": {
    "landTp": true, // 是否启用领地传送
    "maxLand": 20, // 玩家最大领地数量
    "minSpacing": 16, // 领地最小间距
    "refundRate": 0.9, // 退款率(0.0~1.0，1.0为全额退款，0.9为退还90%)
    "discountRate": 1.0, // 折扣率(0.0~1.0，1.0为原价，0.9为打9折)

    "setupDrawCommand": true, // 是否注册领地范围绘制指令
    "drawRange": 64, // 绘制查询领地范围

    "subLand": {
      "enabled": true, // 是否启用子领地
      "maxNested": 5, // 最大嵌套层数(默认5，最大16)
      "minSpacing": 8, // 子领地之间的最小间距
      "maxSubLand": 6, // 每个领地的最大子领地数量
      "calculate": "(square * 8 + height * 20)" // 价格公式
    },

    "tip": {
      "enterTip": true, // 是否启用进入领地提示
      "bottomContinuedTip": true, // 是否启用领地底部持续提示
      "bottomTipFrequency": 1 // 领地底部持续提示频率(s)
    },
    "bought": {
      "threeDimensionl": {
        "enabled": true, // 是否启用三维领地(购买)
        "calculate": "square * 25" // 领地价格计算公式
      },
      "twoDimensionl": {
        "enabled": true, // 是否启用二维领地(购买)
        "calculate": "square * 8 + height * 20" // 领地价格计算公式
      },
      "squareRange": {
        "min": 4, // 领地最小面积
        "max": 60000, // 领地最大面积
        "minHeight": 1 // 领地最小高度
      },
      "allowDimensions": [
        // 允许圈地的维度
        0, 1, 2
      ],
      "forbiddenRanges": [
        // 禁止创建领地的区域，领地管理员可以绕过此限制
        // min: 最小坐标
        // max: 最大坐标
        // dimensionId: 维度ID (0: 主世界, 1: 下界, 2: 末地)
        {
          "aabb": {
            "min": {
              "x": -100,
              "y": 0,
              "z": -100
            },
            "max": {
              "x": 100,
              "y": 255,
              "z": 100
            }
          },
          "dimensionId": 0
        }
      ],
      "dimensionPriceCoefficients": {
        // 维度价格系数，用于根据维度ID调整领地价格。键是维度ID的字符串形式，值是对应的价格系数。
        // 例如，"0": 1.0 表示主世界价格系数为1.0（原价），"1": 1.2 表示下界价格为1.2倍。
        "0": 1.0,
        "1": 1.2,
        "2": 1.5
      }
    }
  },
  "selector": {
    "tool": "minecraft:stick" // 选择工具
  },
  "listeners": {
    // 事件监听器开关 true 为开启，false 为关闭
    // 注意：非必要情况下，请勿关闭事件监听器，否则可能导致领地功能异常
    // 如果您不知道某个事件监听器是做什么的，请不要关闭它
    // 提供的注释仅概括了大致作用，具体行为请查看源码
    "ActorHurtEvent": true, // 实体受伤事件
    "PlayerDestroyBlockEvent": true, // 玩家破坏方块事件
    "PlayerPlacingBlockEvent": true, // 玩家放置方块事件
    "PlayerInteractBlockEvent": true, // 玩家交互方块事件(使用物品)
    "FireSpreadEvent": true, // 火势蔓延事件
    "PlayerAttackEvent": true, // 玩家攻击事件
    "PlayerPickUpItemEvent": true, // 玩家拾取物品事件
    "PlayerInteractBlockEvent1": true, // 玩家交互方块事件(功能性方块)
    "PlayerUseItemEvent": true, // 玩家使用物品事件
    "PlayerAttackBlockBeforeEvent": true, // 玩家攻击方块事件
    "ArmorStandSwapItemBeforeEvent": true, // 盔甲架交换物品事件
    "PlayerDropItemBeforeEvent": true, // 玩家丢弃物品事件
    "ActorRideBeforeEvent": true, // 实体骑乘事件
    "ExplosionBeforeEvent": true, // 爆炸事件
    "FarmDecayBeforeEvent": true, // 农田枯萎事件
    "MobHurtEffectBeforeEvent": true, // 生物受伤效果事件
    "PistonPushBeforeEvent": true, // 活塞推动事件
    "PlayerOperatedItemFrameBeforeEvent": true, // 玩家操作物品展示框事件
    "ActorTriggerPressurePlateBeforeEvent": true, // 实体触发压力板事件
    "ProjectileCreateBeforeEvent": true, // 投掷物创建事件
    "RedstoneUpdateBeforeEvent": true, // 红石更新事件
    "WitherDestroyBeforeEvent": true, // 凋零破坏事件
    "MossGrowthBeforeEvent": true, // 苔藓生长事件
    "LiquidTryFlowBeforeEvent": true, // 液体尝试流动事件
    "SculkBlockGrowthBeforeEvent": true, // 诡秘方块生长事件
    "SculkSpreadBeforeEvent": true, // 诡秘蔓延事件
    "PlayerEditSignBeforeEvent": true, // 玩家编辑告示牌事件
    "SpawnedMobEvent": true, // 生物生成事件(怪物和动物)
    "SculkCatalystAbsorbExperienceBeforeEvent": false, // 幽匿催化体吸收经验事件
    "PlayerInteractEntityBeforeEvent": true, // 实体交互事件
    "BlockFallBeforeEvent": true, // 方块下落事件
    "ActorDestroyBlockEvent": true, // 实体破坏方块事件
    "EndermanLeaveBlockEvent": true, // 末影人搬走方块
    "EndermanTakeBlockEvent": true, // 末影人放下方块
    "DragonEggBlockTeleportBeforeEvent": true // 龙蛋传送事件
  },
  // v0.8.2
  "mob": {
    "hostileMobTypeNames": [
      // 敌对生物
      // 关联权限: "allowMonsterDamage": "允许敌对生物受伤",
      "minecraft:enderman",
      "minecraft:zombie",
      "minecraft:witch",
      "minecraft:endermite",
      "minecraft:skeleton",
      "minecraft:zoglin",
      "minecraft:creeper",
      "minecraft:ghast",
      "minecraft:spider",
      "minecraft:blaze",
      "minecraft:magma_cube",
      "minecraft:silverfish",
      "minecraft:slime",
      "minecraft:pillager",
      "minecraft:guardian",
      "minecraft:vex",
      "minecraft:wither",
      "minecraft:elder_guardian",
      "minecraft:wither_skeleton",
      "minecraft:stray",
      "minecraft:husk",
      "minecraft:hoglin",
      "minecraft:zombie_villager",
      "minecraft:drowned",
      "minecraft:phantom",
      "minecraft:vindicator",
      "minecraft:ravager",
      "minecraft:evocation_illager",
      "minecraft:shulker",
      "minecraft:cave_spider",
      "minecraft:piglin_brute",
      "minecraft:ender_dragon"
    ],
    "specialMobTypeNames": [
      // 特殊生物 (在 hostileMobTypeNames 后检查)
      // 关联权限: "allowSpecialDamage": "允许特殊实体受伤(船、矿车、画等)",
      "minecraft:fox",
      "minecraft:iron_golem",
      "minecraft:parrot",
      "minecraft:cat",
      "minecraft:bee",
      "minecraft:wolf",
      "minecraft:wandering_trader",
      "minecraft:snow_golem",
      "minecraft:villager",
      "minecraft:dolphin",
      "minecraft:llama",
      "minecraft:trader_llama",
      "minecraft:tropicalfish",
      "minecraft:turtle",
      "minecraft:panda",
      "minecraft:polar_bear",
      "minecraft:pufferfish",
      "minecraft:squid",
      "minecraft:salmon",
      "minecraft:cod",
      "minecraft:glow_squid",
      "minecraft:axolotl",
      "minecraft:goat",
      "minecraft:frog",
      "minecraft:allay",
      "minecraft:strider"
    ],
    "passiveMobTypeNames": [
      // 友好/中立生物 (在 specialMobTypeNames 后检查)
      // 关联权限: "allowPassiveDamage": "允许友好、中立生物受伤",
      "minecraft:sheep",
      "minecraft:cow",
      "minecraft:rabbit",
      "minecraft:pig",
      "minecraft:chicken",
      "minecraft:mooshroom",
      "minecraft:horse",
      "minecraft:donkey",
      "minecraft:mule",
      "minecraft:ocelot",
      "minecraft:bat",
      "minecraft:sniffer",
      "minecraft:camel",
      "minecraft:armadillo"
    ],
    "customSpecialMobTypeNames": [
      // 自定义特殊生物、例如 Addon 生物 (最后检查)
      // 关联权限："allowCustomSpecialDamage": "允许自定义实体受伤",
    ]
  },
  "internal": {
    "devTools": false // 是否启用开发工具，启用前请确保您的机器有具有显示器，否则初始化时会引发错误、甚至崩溃。
  }
}
```

## calculate 计算公式

?> PLand 的 `Calculate` 实现使用了 [`exprtk`](https://github.com/ArashPartow/exprtk) 库，因此你可以使用 [`exprtk`](https://github.com/ArashPartow/exprtk) 库所支持的所有函数和运算符。

|     变量      |     描述     |
| :-----------: | :----------: |
|   `height`    |   领地高度   |
|    `width`    |   领地宽度   |
|    `depth`    | 领地深度(长) |
|   `square`    |   领地面积   |
|   `volume`    |   领地体积   |
| `dimensionId` |   维度 ID    |

除此之外，价格表达式还支持调用随机数。

!> 注意：此功能仅限 `v0.8.0` 及以上版本使用。

|        函数        |             原型             |  返回值  |                描述                |
| :----------------: | :--------------------------: | :------: | :--------------------------------: |
|    `random_num`    |        `random_num()`        | `double` |   返回一个 `[0, 1)` 之间的随机数   |
| `random_num_range` | `random_num_range(min, max)` | `double` | 返回一个 `[min, max)` 之间的随机数 |

那么，我们可以写出这样的价格表达式：

```js
"square * random_num_range(10, 50)"; // 领地面积乘以 `[10, 50)` 之间的随机数
```
