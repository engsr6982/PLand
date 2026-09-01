# 领地配置文件

PLand 的所有配置文件均为 **JSON 格式**，位于 `plugins/PLand/config` 目录下。

::: warning 编辑提示
- 请使用支持 JSON 的编辑器（如 VS Code、Notepad++）编辑，**不要使用记事本**，否则可能损坏配置文件。
- 修改后执行 `/pland reload` 热重载（注意：热重载并非完全彻底，推荐使用 `ll reactivate PLand` 完全重载）。
:::

## 配置总览

| 配置项 | 作用 | 位置 |
|:------|:----|:----|
| [`economy`](#economy-经济系统) | 经济系统开关与后端选择 | `Config.json` |
| [`selector`](#selector-选区工具) | 圈地选点工具设置 | `Config.json` |
| [`features`](#features-功能开关) | 领地传送、领地绘制、进服提示等 | `Config.json` |
| [`constraints`](#constraints-全局约束) | 全服统一的领地规则（数量/大小/间距） | `Config.json` |
| [`business`](#business-商业配置) | 买断制 / 租赁制的价格与规则 | `Config.json` |
| [`system`](#system-系统配置) | 遥测、开发者工具 | `Config.json` |
| [`listeners` / `hooks` / `rules`](#) | 事件监听、权限修补、特殊物品权限 | `InterceptorConfig.json`（[单独文档](/user_guide/InterceptorConfig)） |

## 完整配置示例

以下为 `Config.json` 的完整示例（含注释说明）：

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

## economy 经济系统

控制领地买卖所使用的经济后端。

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `enabled` | `false` | 是否启用经济系统（**未启用时购买/租赁不消耗游戏币**，可免费进行） |
| `kit` | `LegacyMoney` | 经济后端：`LegacyMoney`（LLMoney）或 `ScoreBoard`（计分板） |
| `scoreboardName` | `Scoreboard` | 使用计分板后端时，计分板对象名称 |
| `economyName` | `Coin` | 经济名称（显示用） |

::: warning 使用 `LegacyMoney` 时，需要额外安装 [LegacyMoney](https://github.com/LiteLDev/LegacyMoney) 插件。
:::

## selector 选区工具

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `item` | `minecraft:stick` | 用于圈地选点的物品（填写物品命名空间 ID） |
| `alias` | `木棍` | 物品别名（提示信息中使用） |

## features 功能开关

### 领地传送 `landTeleport`

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `landTeleport` | `true` | 是否允许玩家传送到领地传送点 |

### 领地绘制 `draw`

在游戏内用颜色线条显示领地边界（需要安装 DebugShape 或使用默认粒子）。

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `enabled` | `false` | 是否注册 `/pland draw` 命令（**不影响**选区自动渲染边框） |
| `range` | `64` | 单次绘制的范围（以玩家为中心） |
| `backend` | `DebugShape` | 绘制后端：`DebugShape` 或 `DefaultParticle` |
| `color.*` | 见上 | 各场景下的边界颜色（仅 `DebugShape` 后端有效） |

::: tip 注意
- **圈地时选区的边框是自动渲染的**，不依赖 `draw.enabled` 开关
- `features.draw.enabled` 只控制 `/pland draw` 命令是否注册（用于手动查看附近领地的边界）
:::

### 进服/进地提示 `notifications`

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `enterLandTip` | `true` | 进入领地时是否显示提示 |
| `bottomContinuousTip` | `true` | 是否在底部持续显示当前领地信息 |
| `bottomTipCycle` | `1` | 底部提示的刷新周期（秒） |

## constraints 全局约束

> 此配置影响**所有玩家和领地**。

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `maxLandsPerPlayer` | `20` | 每个玩家最多可拥有的领地数量 |
| `nameRule.minLen` | `1` | 领地名最小长度 |
| `nameRule.maxLen` | `32` | 领地名最大长度 |
| `nameRule.allowNewline` | `false` | 领地名是否允许换行符 |
| `size.minSideLength` | `4` | 领地最小边长（X 或 Z 轴） |
| `size.maxSideLength` | `60000` | 领地最大边长（X 或 Z 轴） |
| `size.minHeight` | `1` | 领地最小高度（Y 轴） |
| `spacing.minDistance` | `16` | 独立领地之间的最小间距 |
| `spacing.includeY` | `true` | 间距计算是否包含 Y 轴 |
| `forbiddenRanges` | `[]` | 禁止创建领地的区域列表 |
| `leaseOnlyRanges` | `[]` | 仅允许租赁（不可买断）的区域列表 |

## business 商业配置

### 价格倍率与折扣

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `dimensionalPriceMultiplier` | `{}` | 各维度价格倍率（key 为维度 ID，value 为倍率） |
| `discountRate` | `1.0` | 全局折扣率（**0.01 ~ 1.0**，最后环节相乘） |

### 买断制 `bought`

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `refundRate` | `0.9` | 退款率（**0.01 ~ 1.0**，越高退款越多） |
| `allowDimensions` | `[0,1,2]` | 允许购买领地的维度列表 |
| `mode3D.enabled` | `true` | 是否允许购买 3D 立体领地 |
| `mode3D.formula` | `square * 8 + height * 20` | 3D 领地价格公式 |
| `mode2D.enabled` | `true` | 是否允许购买 2D 平面领地 |
| `mode2D.formula` | `square * 25` | 2D 领地价格公式 |
| `subLand.enabled` | `false` | 是否启用子领地功能 |
| `subLand.maxNestedDepth` | `6` | 子领地最大嵌套深度（最大 16） |
| `subLand.maxSubLandsPerLand` | `6` | 每个领地最多可创建的子领地数量 |
| `subLand.minSpacing` | `8` | 子领地之间的最小间距 |
| `subLand.minSpacingIncludeY` | `true` | 子领地间距是否包含 Y 轴 |
| `subLand.formula` | `square * 8 + height * 20` | 子领地价格公式 |

### 租赁制 `leasing`

> 租赁制详细设计见 [设计文档：租赁模式](/design/feat/LeasingModel)。

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `enabled` | `false` | 是否启用租赁功能 |
| `allowDimensions` | `[0,1,2]` | 允许租赁领地的维度列表 |
| `mode3D.enabled` | `true` | 是否允许租赁 3D 领地 |
| `mode3D.formula` | `(square * 2 + height * 5)` | 3D 领地每日租金公式 |
| `mode2D.enabled` | `true` | 是否允许租赁 2D 领地 |
| `mode2D.formula` | `(square * 8)` | 2D 领地每日租金公式 |
| `duration.minPeriod` | `7` | 最小租期（天） |
| `duration.maxPeriod` | `30` | 最大租期（天） |
| `duration.renewalAdvance` | `3` | 允许提前续约的天数 |
| `freeze.days` | `7` | 到期后的冻结天数 |
| `freeze.penaltyRatePerDay` | `0.05` | 冻结期间每日罚金比例（5%） |
| `recycle.mode` | `TransferToSystem` | 领地回收方式：`TransferToSystem`（转给系统）或 `Delete`（删除） |
| `recycle.keepMembers` | `false` | 回收时是否保留成员（仅 `TransferToSystem` 有效） |
| `notifications.loginTip` | `true` | 登录时是否提示租赁状态 |
| `notifications.enterTip` | `true` | 进入领地时是否提示租赁状态 |

## system 系统配置

| 配置项 | 默认值 | 说明 |
|:------|:-----|:----|
| `telemetry` | `true` | 是否启用遥测统计（[了解更多](/user_guide/FAQ#关于遥测)） |
| `devTools` | `false` | 是否启用开发者工具（依赖 OpenGL3 与 Windows 桌面环境） |

## formula 价格表达式

价格公式是 PLand 的核心玩法配置之一，支持使用以下**变量**和**函数**动态计算价格。

::: tip PLand 使用 [`exprtk`](https://github.com/ArashPartow/exprtk) 库实现价格表达式，因此你可以使用 [
`exprtk`](https://github.com/ArashPartow/exprtk) 库所支持的所有函数和运算符。
:::

### 可用变量

|      常量       |   描述    |
|:-------------:|:-------:|
|   `height`    |  领地高度   |
|    `width`    |  领地宽度   |
|    `depth`    | 领地深度(长) |
|   `square`    |  领地面积   |
|   `volume`    |  领地体积   |
| `dimensionId` |  维度 ID  |

### 随机数函数

::: warning 注意：以下功能仅限 `v0.8.0` 及以上版本使用。
:::

|         函数         |              原型              |   返回值    |            描述            |
|:------------------:|:----------------------------:|:--------:|:------------------------:|
|    `random_num`    |        `random_num()`        | `double` |   返回一个 `[0, 1)` 之间的随机数   |
| `random_num_range` | `random_num_range(min, max)` | `double` | 返回一个 `[min, max)` 之间的随机数 |

### 示例

```js
"square * random_num_range(10, 50)"; // 领地面积乘以 `[10, 50)` 之间的随机数
```
