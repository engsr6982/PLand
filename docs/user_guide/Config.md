# 领地配置文件

> 配置文件均为`json`格式，位于`plugins/PLand/config`目录下。

::: warning 配置文件为`json`格式，请勿使用记事本等不支持`json`格式的文本编辑器进行编辑，否则会导致配置文件损坏。
:::

## Config.json

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

## formula 价格表达式

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
