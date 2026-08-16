# 配置文件

> 配置文件均为`json`格式，位于`plugins/PLand/config`目录下。

::: warning 配置文件为`json`格式，请勿使用记事本等不支持`json`格式的文本编辑器进行编辑，否则会导致配置文件损坏。
:::

## 领地配置 `Config.json`

```json
{
    "version": 34, // 配置文件版本，请勿修改
    "economy": {
        "enabled": false, // 是否启用经济系统
        "kit": "LegacyMoney", // 经济套件 LegacyMoney 或 ScoreBoard
        "scoreboardName": "Scoreboard", // 计分板对象名称
        "economyName": "Coin" // 经济名称
    },
    "selector": {
        "item": "minecraft:stick", // 用于选点的物品
        "alias": "木棍" // 物品别名
    },
    "features": {
        "landTeleport": true, // 是否启用领地传送
        "draw": {
            "enabled": false, // 是否启用领地绘制
            "range": 64, // 单次绘制范围 (以玩家为中心)
            "backend": "DebugShape", // 绘制后端 DebugShape 或 DefaultParticle

            // 颜色配置，仅在绘制后端为 DebugShape 时有效
            "color": {
                "onSelectorConfirm": "#FF00FF",             // 选区确认后的颜色
                "onResizeLandDrawOldRange": "#FF0000",      // 调整领地大小时绘制旧范围的颜色
                "onCreateSubLandDrawParentLand": "#FFFF00", // 创建子领地时绘制父领地的颜色
                "onUseCommandDrawCurrentLand": "#00FF00",   // 使用命令时绘制当前领地的颜色
                "onUseCommandDrawNearLand": "#00FFFF",      // 使用命令时绘制附近领地的颜色
            }
        },
        "notifications": {
            "enterLandTip": true, // 进入领地提示
            "bottomContinuousTip": true, // 底部持续提示
            "bottomTipCycle": 1 // 底部提示刷新周期(秒)
        }
    },
    
    /**
     * 全局约束配置
     * - 此配置影响所有玩家、领地
     */
    "constraints": {
        "maxLandsPerPlayer": 20, // 每个玩家最大领地数量
        "nameRule": {
            "minLen": 1, // 领地名最小长度
            "maxLen": 32, // 领地名最大长度
            "allowNewline": false // 是否允许换行符
        },
        "size": {
            "minSideLength": 4, // 最小领地边长(X或Z轴的最小跨度)
            "maxSideLength": 60000, // 最大领地边长 (X或Z轴的最大跨度)
            "minHeight": 1 // 最小领地高度 (Y轴跨度)
        },
        "spacing": {
            "minDistance": 16, // 独立领地之间的最小间距
            "includeY": true // 间距计算是否包含 Y 轴
        },
        "forbiddenRanges": [] // 禁止创建领地的范围
        "leaseOnlyRanges": [] // 仅租赁领地范围
    },
    "business": {
    
        /**
         * 维度价格倍率
         * - key: 维度 ID value: 倍率
         * - 默认倍率为 1.0，即 1 乘以任何数都等于它本身
         * - 在价格计算时，PLand 会将上一步的价格乘以这个倍率交给下一步继续计算
         */
        "dimensionalPriceMultiplier": {},
        
        /**
         * 折扣率
         * @note 此配置将在价格计算的最后一个环节对价格进行相乘
         * @note 有效范围为 0.01 ~ 1.0
         */
        "discountRate": 1.0,
        
        "bought": {
            /**
             * 退款率
             * @note 有效范围为 0.01 ~ 1.0
             * @note 退款率越高，玩家退款金额越多
             */
            "refundRate": 0.9,
            "allowDimensions": [ // 允许购买的土地维度列表
                0,
                1,
                2
            ],
            "mode3D": { // 3D 立体领地购买配置
                "enabled": true, // 是否启用
                "formula": "square * 8 + height * 20" // 价格公式
            },
            "mode2D": { // 2D 平面领地购买配置
                "enabled": true,
                "formula": "square * 25"
            },
            "subLand": {
                "enabled": false, // 是否启用子领地特性
                "maxNestedDepth": 6, // 子领地最大嵌套深度(最大16)
                "maxSubLandsPerLand": 6, // 每个领地最多可创建多少个子领地
                "minSpacing": 8, // 子领地之间的最小间距
                "minSpacingIncludeY": true, // 子领地之间的最小间距是否包含Y轴
                "formula": "square * 8 + height * 20" // 价格公式
            }
        },
        "leasing": {
            "enabled": false, // 是否启用租赁功能
            "allowDimensions": [ // 允许租赁的土地维度列表
                0,
                1,
                2
            ],
            "mode3D": {
                "enabled": true,
                "formula": "(square * 2 + height * 5)"
            },
            "mode2D": {
                "enabled": true,
                "formula": "(square * 8)"
            },
            "duration": {
                "minPeriod": 7, // 最小租期（天）
                "maxPeriod": 30, // 最大租期（天）
                "renewalAdvance": 3 // 续约提前天数
            },
            "freeze": {
                "days": 7, // 冻结持续天数
                "penaltyRatePerDay": 0.05 // 每日罚金比例（5%）
            },
            "recycle": {
                "mode": "TransferToSystem", // 领地回收处理方式 (TransferToSystem | Delete)
                "keepMembers": false, // 回收时是否保留领地成员(仅 TransferToSystem 模式有效)
            },
            "notifications": { // 到期/续期提醒
                "loginTip": true, // 登录时是否显示提示
                "enterTip": true // 进入土地时是否显示提示
            }
        }
    },
    "system": {
        "telemetry": true, // 是否启用遥测
        "devTools": false // 是否启用开发者工具 (此工具依赖 OpenGL3 & Windows 桌面环境，请确保环境支持)
    }
}
```

### formula 价格表达式

::: tip PLand 使用 [`exprtk`](https://github.com/ArashPartow/exprtk) 库实现价格表达式，因此你可以使用 [
`exprtk`](https://github.com/ArashPartow/exprtk) 库所支持的所有函数和运算符。
:::

|      常量       |   描述    |
|:-------------:|:-------:|
|   `height`    |  领地高度   |
|    `width`    |  领地宽度   |
|    `depth`    | 领地深度(长) |
|   `square`    |  领地面积   |
|   `volume`    |  领地体积   |
| `dimensionId` |  维度 ID  |

除此之外，价格表达式还支持调用随机数。

::: warning 注意：以下功能仅限 `v0.8.0` 及以上版本使用。
:::

|         函数         |              原型              |   返回值    |            描述            |
|:------------------:|:----------------------------:|:--------:|:------------------------:|
|    `random_num`    |        `random_num()`        | `double` |   返回一个 `[0, 1)` 之间的随机数   |
| `random_num_range` | `random_num_range(min, max)` | `double` | 返回一个 `[min, max)` 之间的随机数 |

那么，我们可以写出这样的价格表达式：

```js
"square * random_num_range(10, 50)"; // 领地面积乘以 `[10, 50)` 之间的随机数
```

## 拦截器配置 `InterceptorConfig.json`

```json
{
    "version": 2, // 配置文件版本
    "listeners": {
        // 事件监听器开关 true 为开启，false 为关闭
        // 注意：非必要情况下，请勿关闭事件监听器，否则可能导致领地功能异常
        // 如果您不知道某个事件监听器是做什么的，请不要关闭它
        // 提供的注释仅概括了大致作用，具体行为请查看源码
        // 当然，如果某个事件监听器导致游戏崩溃，您也可以选择关闭它
        "PlayerDestroyBlockEvent": true, // 玩家破坏方块事件
        "PlayerPlacingBlockEvent": true, // 玩家放置方块事件
        "PlayerInteractBlockEvent": true, // 玩家交互方块事件
        "PlayerAttackEvent": true, // 玩家攻击事件
        "PlayerPickUpItemEvent": true, // 玩家拾取物品事件
        "SpawnedMobEvent": true, // 生成生物事件
        "ActorHurtEvent": true, // 生物受伤事件
        "FireSpreadEvent": true, // 火焰蔓延事件
        "ActorDestroyBlockEvent": true, // 生物破坏方块事件
        "MobTakeBlockBeforeEvent": true, // 生物取方块事件
        "MobPlaceBlockBeforeEvent": true, // 生物放置方块事件
        "ActorPickupItemBeforeEvent": true, // 生物拾取物品事件
        "ActorRideBeforeEvent": true, // 生物骑乘事件
        "MobHurtEffectBeforeEvent": true, // 生物受伤效果事件
        "ActorTriggerPressurePlateBeforeEvent": true, // 生物触发压力板事件
        "PlayerInteractEntityBeforeEvent": true, // 玩家交互实体事件
        "ArmorStandSwapItemBeforeEvent": true, // 僵尸村民交互事件
        "PlayerDropItemBeforeEvent": true, // 玩家丢弃物品事件
        "PlayerOperatedItemFrameBeforeEvent": true, // 玩家操作物品展示框事件
        "PlayerEditSignBeforeEvent": true, // 玩家编辑告示牌事件
        "ExplosionBeforeEvent": true, // 爆炸事件
        "PistonPushBeforeEvent": true, // 活塞推动事件
        "RedstoneUpdateBeforeEvent": true, // 红石更新事件
        "BlockFallBeforeEvent": true, // 方块掉落事件
        "WitherDestroyBeforeEvent": true, // 凋落破坏事件
        "MossGrowthBeforeEvent": true, // 苔藓生长事件
        "LiquidFlowBeforeEvent": true, // 液体流动事件
        "DragonEggBlockTeleportBeforeEvent": true, // 龙蛋方块传送事件
        "SculkBlockGrowthBeforeEvent": true, // 藤蔓生长事件
        "SculkSpreadBeforeEvent": true, // 藤蔓蔓延事件
        "PlayerUseItemEvent": true // 玩家使用物品事件
    },
    "hooks": {
        // Hook 技术指的是在软件运行过程中，通过拦截、修改或补充原有代码逻辑，实现对目标软件行为的影响和控制的技术手段
        // 这里的 Hook 用于 Patch 修补一些领地越权问题
        // 每个 Hook 对应一个或多个权限修复，true 为开启，false 为关闭
        // 注意：非必要情况下，请勿关闭 Hook，否则可能导致领地功能异常
        // 如果您不知道某个 Hook 是做什么的，请不要关闭它
        // 提供的注释仅概括了大致作用，具体行为请查看源码
        // 当然，如果 Hook 导致游戏崩溃，您也可以选择关闭它
        "FishingHookHitHook": true, // 钓鱼钩击中
        "LayEggGoalHook": true, // 海龟产卵
        "FireBlockBurnHook": true, // 火焰燃烧方块
        "ChestBlockActorOpenHook": true, // 箱子实体打开 (铜傀儡)
        "LightningBoltHook": true, // 闪电
        "LecternBlockUseHook": true, // 使用讲台
        "LecternBlockDropBookHook": true, // 取出讲台书本
        "OozingMobEffectHook": true, // 渗浆效果
        "WeavingMobEffectHook": true, // 盘丝效果
        "HopperComponentPullInItemsHook": true, // 漏斗组件吸取物品(漏斗矿车)
        "ExperienceOrbPlayerTouchHook": true, // 经验球拾取
        "ThrownTridentPlayerTouchHook": true, // 三叉戟拾取
        "ArrowPlayerTouchHook": true, // 箭矢拾取
        "AbstractArrowPlayerTouchHook": true, // 箭类投射物拾取
        "FarmChangeEventHook": true // 农田踩踏/退化
    },
    "rules": {
        "mob": {
            "allowHostileDamage": [
                // 允许敌对生物受伤白名单
                "minecraft:creeper",
                "minecraft:enderman",
                "minecraft:zombie",
                "minecraft:drowned"
                // ....
            ],
            "allowFriendlyDamage": [
                // 允许友好(中立)生物受伤白名单
                "minecraft:armadillo",
                "minecraft:sheep",
                "minecraft:cow",
                "minecraft:rabbit"
                // ....
            ],
            "allowSpecialEntityDamage": [
                // 允许特殊生物受伤白名单
                // 自定义特殊生物、例如 Addon 生物
                "minecraft:painting",               // 画
                "minecraft:hopper_minecart",        // 漏斗矿车
                "minecraft:chest_boat",             // 箱船
                "minecraft:leash_knot"             // 拴绳结
                // ......
            ]
        },
        "item": {
            "minecraft:skull": "allowPlace",
            "minecraft:banner": "allowPlace",
            "minecraft:glow_ink_sac": "allowPlace",
            "minecraft:end_crystal": "allowPlace"
            // ...
        },
        "block": {
            "minecraft:noteblock": "useNoteBlock",
            "minecraft:crafter": "useCrafter",
            "minecraft:soul_campfire": "useCampfire",
            "minecraft:chest": "allowOpenChest",
            "minecraft:anvil": "useAnvil"
            // ....
        }
    }
}
```

### `rules.item` 和 `rules.block`

此部分用于配置特定物品或方块交互所需的权限。这允许服主自定义哪些物品/方块在领地内可以被非成员使用。

- `rules.item`: 定义了使用特定**物品**时所需的权限。键是物品的命名空间 ID，值是权限名称。
- `rules.block`: 定义了与特定**方块**交互时所需的权限（通常是简单交互，如使用床、工作台、熔炉等）。键是方块的命名空间
  ID，值是权限名称。

默认配置中包含了大部分原版物品和方块的权限设置，您可以根据需要进行修改、添加或删除。

### 可用的权限值

以下是所有可用于 `rules.item` 和 `rules.block` 的权限名称字符串：

| 权限字段                       | 注释                                          |
|----------------------------|---------------------------------------------|
| `allowDestroy`             | 允许破坏方块                                      |
| `allowPlace`               | 允许放置方块                                      |
| `useBucket`                | 允许使用桶(水/岩浆/...)                             |
| `useAxe`                   | 允许使用斧头                                      |
| `useHoe`                   | 允许使用锄头                                      |
| `useShovel`                | 允许使用铲子                                      |
| `placeBoat`                | 允许放置船                                       |
| `placeMinecart`            | 允许放置矿车                                      |
| `useButton`                | 允许使用按钮                                      |
| `useDoor`                  | 允许使用门                                       |
| `useFenceGate`             | 允许使用栅栏门                                     |
| `allowInteractEntity`      | 允许与实体交互                                     |
| `useTrapdoor`              | 允许使用活板门                                     |
| `editSign`                 | 允许编辑告示牌                                     |
| `useLever`                 | 允许使用拉杆                                      |
| `useFurnaces`              | 允许使用所有熔炉类方块（熔炉/高炉/烟熏炉）                      |
| `allowPlayerPickupItem`    | 允许玩家拾取物品                                    |
| `allowRideTrans`           | 允许骑乘运输工具（矿车/船）                              |
| `allowRideEntity`          | 允许骑乘实体                                      |
| `usePressurePlate`         | 触发压力板                                       |
| `allowFishingRodAndHook`   | 允许使用钓鱼竿和鱼钩                                  |
| `allowUseThrowable`        | 允许使用投掷物(雪球/鸡蛋/三叉戟/...)                      |
| `useArmorStand`            | 允许使用盔甲架                                     |
| `allowDropItem`            | 允许丢弃物品                                      |
| `useItemFrame`             | 允许操作物品展示框                                   |
| `useFlintAndSteel`         | 使用打火石                                       |
| `useBeacon`                | 使用信标                                        |
| `useBed`                   | 使用床                                         |
| `allowPvP`                 | 允许PvP                                       |
| `allowHostileDamage`       | 敌对生物受到伤害                                    |
| `allowFriendlyDamage`      | 友好生物受到伤害                                    |
| `allowSpecialEntityDamage` | 特殊生物受到伤害                                    |
| `useContainer`             | 允许使用容器(箱子/木桶/潜影盒/发射器/投掷器/漏斗/雕纹书架/试炼宝库/...)  |
| `useWorkstation`           | 工作站类(工作台/铁砧/附魔台/酿造台/锻造台/砂轮/织布机/切石机/制图台/合成器) |
| `useBell`                  | 使用钟                                         |
| `useCampfire`              | 使用营火                                        |
| `useComposter`             | 使用堆肥桶                                       |
| `useDaylightDetector`      | 使用阳光探测器                                     |
| `useJukebox`               | 使用唱片机                                       |
| `useNoteBlock`             | 使用音符盒                                       |
| `useCake`                  | 吃蛋糕                                         |
| `useComparator`            | 使用红石比较器                                     |
| `useRepeater`              | 使用红石中继器                                     |
| `useLectern`               | 使用讲台                                        |
| `useCauldron`              | 使用炼药锅                                       |
| `useRespawnAnchor`         | 使用重生锚                                       |
| `useBoneMeal`              | 使用骨粉                                        |
| `useBeeNest`               | 使用蜂巢(蜂箱)                                    |
| `editFlowerPot`            | 编辑花盆                                        |
| `allowUseRangedWeapon`     | 允许使用远程武器(弓/弩)                               |

**示例**：
默认情况下，`minecraft:flint_and_steel`（打火石）需要 `useFlintAndSteel` 权限。如果您想让它需要 `allowPlace` 权限，您可以这样修改：

```jsonc
{
  // ...
  "rules": {
    // ...
    "item": {
      "minecraft:flint_and_steel": "allowPlace"
      // ...
    }
    // ...
  }
}
```
