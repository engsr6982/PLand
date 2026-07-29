# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### ✨ 新增功能

- 支持自定义领地边框绘制颜色 @engsr6982 [#224](https://github.com/IceBlcokMC/PLand/issues/224)

### 🚀 性能优化

- 改进对实体受伤、玩家攻击时的动态实体分组，减少不必要的多次查表并优化调试日志 @engsr6982

### 🐛 问题修复

- 修复 PC 端选区时有概率多次触发选择 @engsr6982 [#228](https://github.com/IceBlcokMC/PLand/issues/228)

## [0.21.1] - 2026-07-21

### 🐛 问题修复

- **修复拦截器及权限加载异常**：解决因反射损坏导致插件启动时无法识别物品与方块权限规则、引发领地保护失效的问题。 @engsr6982

## [0.21.0] - 2026-07-12

### 🐛 问题修复

- 修复玩家在余额不足的情况下购买领地成功 [#217] @engsr6982 @yangyangzhong82

### 🧩 其他改动

- 补充遗漏的方块权限 [#221] @engsr6982
- 适配 Bedrock Server v26.20 (LeviLamina v26.20.x) @yangyangzhong82 @engsr6982

## [0.20.3] - 2026-06-09

### 🐛 问题修复

- Fix(LeasingService): 修复 mod 关闭时空指针访问异常 [#211] @engsr6982

## [0.20.2] - 2026-04-12

### 🐛 问题修复

- 修复领地保存不成功的问题 #209 @yangyangzhong82

## [0.20.1] - 2026-04-12

### 🐛 问题修复

- 修复在LeviLamina v26.10.5以上的版本不能使用的问题 @yangyangzhong82

## [0.20.0] - 2026-04-08

### 🧩 其他改动

- 适配 Bedrock Sverver v1.26.10 (LeviLamina v26.10.x) @yangyangzhong82
- 补充拦截器配置 @yangyangzhong82
- 移除 MobHurtHook @yangyangzhong82

## [0.19.0-rc.1] - 2026-03-25

### 🐛 问题修复

- 修复继承错误导致的 Land 部分接口调用崩溃 @engsr6982

## [0.19.0-beta.1] - 2026-03-27

### ✨ 新增功能

- 租赁模式 @engsr6982 #154
- 数据库快照 @engsr6982 #165
- 添加**大型垂滴叶触发权限** @engsr6982 #192

### 🐛 问题修复

- 修复 DevTool 领地编辑器非等宽字体显示错位 @engsr6982
- 提高防抖阈值修复PC端选区时可能出现的多次触发 @engsr6982 #199

### 🚀 性能优化

- 新增领地 Owner、Member 索引优化性能 @engsr6982 #204

### 🧩 其他改动

- 重构配置文件结构 @engsr6982

### 🛠️ 开发者相关

新增事件:

- `PlayerLeaseLandEvent` - 玩家租赁领地事件
- `PlayerRenewLandEvent` - 玩家续租领地事件
- `LandStateChangedEvent` - 领地状态变更事件
- `LandRecycleEvent` - 领地被回收事件
- `MembersClearedEvent` - 领地成员清空事件

## [0.18.2] - 2026-03-01

### ✨ 新增功能

- 增强拾取物品权限，其他非掉落物可拾取实体(三叉戟等)拾取需要权限 @yangyangzhong82 #193

### 🐛 问题修复

- 修复领地价格计算错误的问题 @engsr6982 #189
- 修复耕地环境权限无法防止自然退化的问题 @yangyangzhong82 #191
- 修复PlayerDropItemBeforeEvent报错 @yangyangzhong82 #157
- 修复其他模组监听 ActorHurtEvent 会重复监听两次的问题 @yangyangzhong82 #196

## [0.18.1] - 2026-02-15

### 🐛 问题修复

- 修复部分事件误拦截玩家操作的问题 (`MobHurtEffectBeforeEvent`,`ActorHurtEvent`,`PlayerAttackEvent`,
  `PlayerInteractBlockEvent`恢复为黑名单模式) @engsr6982
- 将漏斗矿车行为拆分为独立权限，避免影响生电机器运作 @engsr6982

## [0.18.0] - 2026-02-14

> ⚠️ 本次版本为权限系统重构版本，存在破坏性变更
>
> - 旧版领地权限数据与新版本完全不兼容
> - 插件会在启动时自动执行数据迁移
> - 由于权限模型发生变化，迁移后的权限可能与旧版行为不完全一致

### ✨ 新增功能

- 新增领地 **名称**、**描述** 可配置检查 @engsr6982
- DevTool 新增领地树可视化 @engsr6982
- 管理 GUI：
  - 管理玩家表单支持分页和搜索（#129）@engsr6982
  - 支持按领地 ID 查找领地 @engsr6982
- 权限系统:
  - 区分环境权限与角色权限 #170 @engsr6982
  - 成员支持独立权限配置 #170 @engsr6982
- 添加实体拾取物品事件(关联`allowMobGrief`权限) #171 @engsr6982
- 分离拦截配置(InterceptorConfig.json)和领地配置(Config.json)，改进权限映射 #169 @engsr6982

### 🐛 问题修复

- 修复领地最小间距计算错误 @engsr6982
- 修复领地检查错误信息翻译失败 @engsr6982
- 重命名权限语义解决弹射物吞物品问题 #139 @engsr6982
- 修复远程武器攻击领地内实体问题 #177 @engsr6982
- 修复闪电造成领地内方块、实体状态更新 #167 @engsr6982
- 修复领地价格表达式解析失败时可能导致经济异常结算的问题 @engsr6982
- 修复玩家可以无权限放置、取下讲台书本 #143 @engsr6982
- 修复渗浆和盘丝药水在生物死亡后还是能使用 #59 @engsr6982
- 修复漏斗矿车可以吸取领地内物品 #55 @engsr6982

### 🧩 逻辑优化

- 预计算领地层级缓存，优化子领地查询性能 @engsr6982

### 🧹 其他改动

- 领地管理 GUI 新增 **创建子领地** 按钮 @engsr6982
- 移除 `pland set language` 命令 @engsr6982
- 移除对 **iLand** 领地数据转换支持 @engsr6982
- 移除领地描述相关配置与编辑入口（该功能无实际使用场景）#181 @engsr6982

### 🛠️ 开发者相关

- 重构代码，清理历史代码，引入 Service、pImpl 模式 (#166) @engsr6982
- 调整项目工程结构 @engsr6982
- 改进内置领地表单系统 (#144) @engsr6982

#### 🔔 事件更改

> ⚠️ 本次版本重构了领地事件体系，旧事件已全部移除，不再提供兼容与迁移映射。  
> 请基于以下新增事件重新接入监听逻辑。

- [-] 移除 `LandEvent.h` 内所有事件 @engsr6982
- [+] 新增 `LandResizedEvent` 事件 @engsr6982
- [+] 新增 `MemberChangedEvent` 事件 @engsr6982
- [+] 新增 `OwnerChangedEvent` 事件 @engsr6982
- [+] 新增 `LandRefundFailedEvent` 事件 @engsr6982
- [+] 新增 `PlayerApplyLandRangeChangeBeforeEvent`, `PlayerApplyLandRangeChangeAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerBuyLandBeforeEvent`, `PlayerBuyLandAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerChangeLandDescBeforeEvent`, `PlayerChangeLandDescAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerChangeLandMemberBeforeEvent`, `PlayerChangeLandMemberAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerChangeLandNameBeforeEvent`, `PlayerChangeLandNameAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerDeleteLandBeforeEvent`, `PlayerDeleteLandAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerEnterLandEvent`, `PlayerLeaveLandEvent` 事件 @engsr6982
- [+] 新增 `PlayerRequestChangeLandRangeBeforeEvent`, `PlayerRequestChangeLandRangeAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerTransferLandBeforeEvent`, `PlayerTransferLandAfterEvent` 事件 @engsr6982
- [+] 新增 `PlayerRequestCreateLandEvent` 事件 @engsr6982

#### 💾 领地数据权限字段变更

> 以下 v25 字段在 v27 中不再独立存在：

- `allowActorDestroy` & `allowEndermanLeaveBlock` -> 合并为 `environment.allowMobGrief`。
- `allowSpecialDamage` & `allowCustomSpecialDamage` -> 合并为 `role.allowSpecialEntityDamage`。
- `useFurnace`, `useBlastFurnace`, `useSmoker` -> 合并为 `role.useFurnaces`。
- `allowOpenChest`, `useBarrel`, `useShulkerBox` 等8项 -> 合并为 `role.useContainer`。
- `useCraftingTable`, `useAnvil` 等10项 -> 合并为 `role.useWorkstation`。
- `allowAxePeeled` -> 重命名为 `role.useAxe`。
- `allowPickupItem` -> 重命名为 `role.allowPlayerPickupItem`。
- `allowAttackDragonEgg` -> 重命名为 `environment.allowDragonEggTeleport`。

## [0.17.2] 2026-02-12

### 🐛 问题修复

- 修复领地价格表达式解析失败时可能导致经济异常结算的问题 @engsr6982

## [0.17.1] - 2026-01-28

### 🐛 问题修复

- 修复低版本数据加载失败 @engsr6982

### 🧹 其他改动

- 调整 `PlayerRequestCreateLandEvent::type()` 符号可见性 @engsr6982

## [0.17.0] - 2026-01-27

### 🐛 问题修复

- 调整 mod 析构顺序，修复可能的崩溃 @engsr6982
- 修复配置文件 `minHeight` 配置项无效 @engsr6982
- 修复领地轴方块数量计算错误 @engsr6982

### 🧩 逻辑优化

- 移除对 `BedrockServerClientInterface` 绘制后端的代码支持 @yangyangzhong82
- [#162] 改进领地绘制处理，绘制不再对所有玩家可见 @yangyangzhong82
- 重构部分代码，改进异常、消息处理，优化领地选区防抖处理... @engsr6982

### 🧹 其他改动

- 领地绘制支持热重载 @engsr6982
- 移除 `PlayerAskCreateLandBeforeEvent` 事件 @engsr6982
- 新增 `PlayerRequestCreateLandEvent` 事件 @engsr6982
- 适配 Levilamina v1.9.x @engsr6982

## [0.16.0] - 2025-11-10

### 🐛 问题修复

- [#156] 修复新创建的领地未正常持久化保存 @engsr6982
- [#158] 修复铜傀儡能开箱的问题 @yangyangzhong82
- [#159] 修复钓鱼竿仍然能在无权限钓生物的问题 @yangyangzhong82
- [#163] 修复火焰无法烧毁方块的问题 @yangyangzhong82

### 🧹 其他改动

- [#161] 适配 DebugShape v0.5.0 ABI @engsr6982
- 适配 LeviLamina v1.7.0 @engsr6982
- 适配 iListenAttentively v0.10.0 @engsr6982

## [0.15.0] - 2025-10-21

### ✨ 新增功能

- [#90] 新增遥测功能，匿名统计版本使用情况 @engsr6982

### 🐛 问题修复

- [#136] 临时修复岩浆烧毁领地边缘方块 @yangyangzhong82

### 🧩 逻辑优化

- 优化部分物品、实体判定 @engsr6982
- 重力方块下落更改为仅外部下落进领地内 @engsr6982

### 🧹 其他改动

- 适配 `LeviLamina v1.6.x` & `iListenAttentively v0.9.x` @engsr6982

## [0.14.1] - 2025-10-3

### 🐛 问题修复

- [#149] [#147] 修复 Mod 关闭时事件监听器未卸载导致的报错 @engsr6982
- [#150] 修复伤害拦截异常 @engsr6982

### 🧹 其他改动

- [#148] `LiquidTryFlowBeforeEvent` 更改为 `LiquidFlowBeforeEvent` @engsr6982

## [0.14.0] - 2025-10-2

### 🐛 问题修复

- [#140] 领地外无权限玩家可击杀领地内实体 @engsr6982
- [#139] 修复领地禁止创建弹射物时使用三叉戟导致吞物品 @engsr6982
- [#138] 修复凋灵在领地边缘冲撞可破坏方块 @engsr6982
- 修复生物受伤权限部分生物伤害无效的问题 @yangyangzhong82
- [#69] 当实体破坏权限关闭时，海龟不再能在领地内产卵 @yangyangzhong82
- [#56] 修复玩家使用钓鱼竿仍然能将领地内生物拉出 @yangyangzhong82

### 🧩 逻辑优化

- 移除部分不合理的 using 语句，提高可读性 @engsr6982
- 优化部分事件监听器，统一事件调试输出 @engsr6982
- 插件加载时增加 Levilamina 版本检测，版本不一致时会发出日志警告用户 @yangyangzhong82

### 🧹 其他改动

- 移除 `debug_shape` 相关封装，改为外部独立 **DebugShape.dll** 组件 @engsr6982
- 适配 Levilamina v1.5.2 @yangyangzhong82
- 适配 iListenAttentively v0.8.0 @yangyangzhong82

## [0.13.0] - 2025-8-27

### ✨ 新增功能

- 新增 DebugShape 绘制后端 #135 @engsr6982
- 新增 `land.drawHandleBackend` 配置项，用于选择绘制后端 #135 @engsr6982
- 选择领地表单新增 **重置过滤器** 按钮 @engsr6982

### 🐛 问题修复

- 修复子领地爆炸权限无效 #133 @yangyangzhong82
- 修复拦截液体流动导致刷物品问题 @engsr6982
- 修复移除子领地时可能的崩溃 @engsr6982
- 修复创建子领地 `land.subLand.minSpacingIncludeY` 配置项未生效问题 @engsr6982

### 🧩 权限与逻辑优化

- 优化 `LandRegistry` 线程析构逻辑，降低 Mod 关闭时的等待时间 @engsr6982

## [0.12.0] - 2025-8-4

### ✨ 新增功能

- 将交互权限检查的映射表移至配置文件，不再硬编码 @yangyangzhong82 #130

### 🐛 问题修复

- 修复玩家转让领地能绕过领地最大数量限制的问题 @yangyangzhong82 #131
- 修复未选择领地范围进行购买领地导致崩溃 @engsr6982
- 修复特定情况下使用领地传送可能传送到异常坐标 #127 @engsr6982 @yangyangzhong82

## [0.12.0-rc.2] - 2025-7-19

### 🐛 问题修复

- 对领地传送功能(`isTargetChunkFullyLoaded`)添加空指针检查 @engsr6982
- 修复选区器 2/3 维颠倒错误 @engsr6982
- 修复领地重新选区功能 @engsr6982

## [0.12.0-rc.1] - 2025-7-19

> 基于 0.11.0-rc.1，适配 LeviLamina、iListenAttentively

### 🧹 其他改动

- 适配 LeviLamina v1.4.1 @engsr6982
- 适配 iListenAttentively v0.7.0 @engsr6982

## [0.11.0-rc.1] - 2025-7-19

### 🐛 问题修复

- 修复领地重新选区购买表单价格显示错误 @engsr6982
- 修复未设置领地传送点进行跨纬度传送失败 #106 @engsr6982
- 修复领地最小间距未生效问题 #115 @engsr6982
- 修复领地边界活塞权限的判断问题 #119 @yangyangzhong82
- 修复子领地创建时高度判断的错误 #117 @yangyangzhong82

### ✨ 新增功能

- 管理 GUI 支持分页 #107 @engsr6982
- 领地转让和添加领地成员支持离线玩家添加，添加需要手动输入名称 #77 @yangyangzhong82
- 实现脏数据计数 #95 @engsr6982
- 重构经济系统，LegacyMoney 更改为可选依赖 @engsr6982
- 重构绘制系统，BedrockServerClientInterface 更改为可选依赖 #114 @engsr6982
- 选择领地表单支持**分页、搜索、过滤** #107 #101 @engsr6982
- 新增配置项 `land.minSpacingIncludeY` #115 @engsr6982
- 支持领地管理员修改默认权限配置 #110 @yangyangzhong82 @engsr6982

### 🧹 其他改动

- 调整工程结构并重构代码 #111 @engsr6982
- 调整默认配置文件 #104 @yangyangzhong82
- 抽离领地创建逻辑，并改进错误提示 #102 @engsr6982

## [0.10.0] - 2025-6-18

### 🐛 问题修复

- 修复选区器析构时没有正确清理已渲染的选区 #94 @engsr6982
- 对生物受伤权限进行优化，删除 ActorHurtEvent 事件，并改为仅对玩家的伤害进行判断 #96 #97 @yangyangzhong82

### 🧹 其他改动

- 适配 LeviLamina v1.3.1 @engsr6982
- 适配 iListenattentively v0.6.0 @engsr6982

## [0.9.0] - 2025-6-9

### 🐛 问题修复

- 修复 xmake 打包语言文件路径错误问题 @engsr6982
- 修复新创建的领地数据库误识别为旧版数据库 @engsr6982
- 修复弹射物事件在某些情况下的报错 @yangyangzhong82
- 修复领地传送功能可在部分关闭的情况下使用 #89 @engsr6982

### 🧩 权限与逻辑优化

- 将生物的类别修改为配置文件可自定义的列表，不再依赖 family 来判断生物类别 #84 @yangyangzhong82

### 🧹 其他改动

- 优化领地重叠冲突时的消息提醒，现在会显示和哪些领地冲突 #85 @yangyangzhong82
- `pland reload` 支持重载事件监听器 @engsr6982
- 适配 LeviLamina v1.2.1 & ilistenattentively v0.5.0-rc.1 @engsr6982

## [0.8.1] - 2025-06-01

### 🐛 问题修复

- 修复玩家可在无权限情况下将拴绳系在领地内栅栏上 #72 @engsr6982

---

### 🧹 其他改动

- 添加数据库加载检查、备份 @engsr6982

## [0.8.0] - 2025-06-01

### ✨ 新增功能

- 增加生物交互权限（#43）@yangyangzhong82
- 增加末影人搬运、实体破坏、方块下落事件的权限控制（#44）@yangyangzhong82 @engsr6982
- 增加龙蛋交互事件支持，并修复关闭权限后龙蛋仍然能被交互的问题（#39）@yangyangzhong82
- 增加 getLandsByOwner API 与重载方法@engsr6982
- 领地 UI 表单增加设置传送点设置的按钮@yangyangzhong82
- 增加 使用合成器,雕纹书架,红石比较器,红石中继器,潜影盒五种方块交互权限，并修复权限判断问题 (#48)@yangyangzhong82
- 增加 玩家破坏画、矿车、船的权限控制(#48)@yangyangzhong82
- 增加 放置船和矿车权限 （#41）@yangyangzhong82
- 增加 对特殊实体造成伤害 权限 @engsr6982
- 增加试炼宝库和蜂巢蜂箱的权限控制 #62 #64 @yangyangzhong82
- `PriceCalculate` 支持调用随机数 @engsr6982
- 增加配置选项控制`forbiddenRanges` ，用于禁止普通玩家在某个区域创建领地 #37 @yangyangzhong82
- 增加配置选项控制`dimensionPriceCoefficients` ，用于自定义每个维度的价格系数，默认为 1.0 @yangyangzhong82

---

### 🐛 问题修复

- 修复悬挂告示牌权限判断缺失的问题（#40）@yangyangzhong82
- 修复部分事件未正确取消导致的逻辑穿透问题 @engsr6982
- 修复语言文件打包路径错误问题 @engsr6982
- 修复玩家长时间停留 GUI 导致 `Player` 悬空引用问题@engsr6982
- 修复玩家乘骑实体权限判断的问题，现在以被乘骑实体的坐标来判断领地而非操作玩家的坐标(#50)@yangyangzhong82
- 修复无权限玩家在领地外仍然能用弹射物伤害领地内生物的问题(#50) @yangyangzhong82
- 修复领地主人无法用伤害药水伤害领地生物的问题 @yangyangzhong82
- 修复河豚可对领地生物造成伤害 #68 @engsr6982
- 修复甜浆果丛可被交互 #63 @engsr6982
- 修复 `pland set <a/b>` 命令在未开启选区时访问空指针引发异常 @engsr6982

---

### 🧩 权限与逻辑优化

- 修改生物判断逻辑，将非 monster 类型统一视为动物,避免某些生物没有归类导致判断问题 (#35) @yangyangzhong82
- 优化弹射物权限判定流程，将除了钓鱼竿之外的弹射物权限全部合并至发射弹射物权限中@yangyangzhong82
- 将液体流动事件替换为 `LiquidFlowBeforeEvent`，提升性能 @yangyangzhong82(#44)
- 将活塞和液体流动修改为只对边界判断@yangyangzhong82 @engsr6982
- 优化领地创建时对数量与范围限制的判断：允许领地管理员无视配置文件的限制创建领地 @yangyangzhong82
- 优化权限判定,对部分方块和物品不再单纯使用类型名进行判断，使其判断更加灵活@yangyangzhong82
- `allowAnimalDamage` 更改为 `allowPassiveDamage` 对友好、中立生物造成伤害 @engsr6982
- 重构 `EventListener`、`LandScheduler` 资源管理，采用 RAII 机制 @engsr6982

---

### 🧹 其他改动

- 重构 DevTool 工具模块，提升扩展性与维护性@engsr6982
- 移除已废弃的 Canvas、LandViewer、DataMenu 模块@engsr6982
- 同步语言文件，统一命名规范@engsr6982
- 移除 `zh_Classical.json` 语言文件 -- 千呼万唤始出来，删之 @engsr6982
- 重命名 `PosBase`、`LandPos` 为 `LandPos`、`LandAABB` @engsr6982
- 玩家进入领地时，默认使用领地名作为子标题 #76 @engsr6982

## [0.7.1] - 2025-04-11

### Changed

- 修改 `allowAttackXXXX` i18n 翻译文本 [#29]
- 更新依赖版本 ilistenattentively v0.4.1

### Fixed

- 修复领地管理员管理玩家领地无法添加自己为领地成员 @yangyangzhong82
- 修复关闭领地传送后管理员无法使用领地传送 [#26] @yangyangzhong82
- 修复管理员无法使用 `/pland this` 命令管理玩家领地 [#28] @yangyangzhong82
- 修复 PLand-SDK 依赖问题 [#32] @engsr6982
- 修复使用 `pland draw` 命令渲染领地时，领地删除未移除渲染 [#31] @yangyangzhong82

## [0.7.0] - 2025-03-28

### Added

- 新增 `en_US`、`ru_RU` 语言文件 #17
- 新增 `/pland list op` 命令 #15
- 新增 `/pland set language` 命令 #19
- 新增 `/pland this` 命令
- `Config` 添加 `subLand` 子领地相配置 #18
- 支持子领地

### Changed

- 移除 `Particle`、`LandDraw`
- 优化 `PLand::getLandAt` 查询
- 重构 `Calculate` 为 `PriceCalculate`
- 适配事件库 v0.4.0 #24
- 重构 `PLand DevTools` 开发者工具
- 重构部分代码

### Fixed

- 修复潜影贝无法生成潜影弹 #23
- 修复部分实体无法受到任何伤害 #22
- 修复创造模式无法破坏领地内方块 #20
- 修复无法正常打开末影箱 #25

## [0.6.0] - 2025-03-01

### Changed

- 适配 LeviLamina 1.1.0
- 重构部分代码

## [0.5.1] - 2025-02-21

### Added

- `Config` 新增 `internal.devTools` 配置项

### Fixed

- 修复部分环境下 `DevTools` 初始化失败引发的崩溃 #13

## [0.5.0] - 2025-02-19

### Added

- 领地选区支持再次点击打开购买界面 #9
- 支持热卸载
- 领地管理界面新增传送按钮 #9
- 支持自定义领地传送点 #9
- OP 领地管理支持模糊搜索 #9
- 新增事件监听器开关
- 新增权限 `allowMonsterSpawn`、`allowAnimalSpawn` #11

### Changed

- 优化部分代码
- `allowAttackMob` 更改为 `allowAttackMonster`
- 移除 `AnimalEntityMap`

### Fixed

- 修复与菜单插件可能的兼容性问题
- 修复领地主人无法攻击实体 #10
- 修复领地主任无法投掷弹射物 #10
- 修复可能的告示牌越权编辑问题 #12
- 修复传送失败返回原位置维度错误

## [0.4.1] - 2025-01-30

### Changed

- 适配 iListenAttentively v0.2.3

## [0.4.0] - 2025-01-27

### Changed

- 适配 LeviLamina 1.0.0

### Fixed

- 暂时移除液体事件（事件库 Bug）

## [0.4.0-rc.2] - 2025-01-13

### Changed

- 适配 LeviLamina 1.0.0-rc.3

## [0.4.0-rc.1] - 2025-01-10

### Added

- 新增权限 `allowSculkBlockGrowth`
- 新增玩家个人设置

### Changed

- 适配 LeviLamina 1.0.0-rc.2

### Fixed

- 修复一些 bug (记不清了)

## [0.3.1] - 2024-12-17

### Fixed

- 修复 `ActorHurtEvent` `ActorRideEvent` `FarmDecayEvent` `MobHurtEffectEvent` `PressurePlateTriggerEvent`
  `ProjectileSpawnEvent` `RedstoneUpdateEvent` 事件意外拦截领地外事件。

## [0.3.0] - 2024-12-15

### Added

- 支持从 iLand 导入数据
- LandData 新增 `mIsConvertedLand`、`mOwnerDataIsXUID` 字段
- 新增 `ActorHurtEvent` 事件处理

## [0.2.7] - 2024-12-6

### Fixed

- 修复删除领地时出现回档问题 [#5]

## [0.2.6] - 2024-11-17

### Changed

- `PLand` 部分成员更改为 `private`

### Fixed

- 修复 `PLand::generateLandID` 可能生成重复的 ID
- 修复 `PLand::addLand` 添加失败依然返回 true
- 修复 `LandBuyGui::impl` 购买失败未返还经济

## [0.2.5] - 2024-11-15

### Fixed

- 修复可能的死锁问题
- 修复 PLand 多线程竞争问题

## [0.2.4] - 2024-11-13

### Changed

- `IChoosePlayerFromDB` 表单增加去重
- `PLand` 增加 `std::mutex` 保护资源

## [0.2.3] - 2024-11-2

### Changed

- `/pland op <target: Player>` 添加未找到玩家时错误提示
- `/pland buy` 领地范围不合法提示添加当前范围信息

### Fixed

- 修复 `LandPos::getSquare`、`LandPos::getVolume` 计算错误

## [0.2.2] - 2024-11-1

### Fixed

- 修复购买 2D 领地时范围验证错误

## [0.2.1] - 2024-10-31

### Fixed

- 修复领地绘制引发的崩溃

## [0.2.0] - 2024-10-16

### Added

- 新增领地绘制功能

## [0.1.0] - 2024-9-27

### Added

- 新增 MossSpreadEvent 事件处理
- 新增 PistonTryPushEvent 事件处理
- 新增 LiquidFlowEvent 事件处理
- 新增 SculkCatalystAbsorbExperienceEvent 事件处理
- 新增 allowLiquidFlow 权限

## [0.0.3] - 2024-9-21

### Added

- 完成权限翻译

### Changed

- 购买领地后，立即清除标题
- 修改 3D 选区时调整 Y 轴 GUI 错误提示

### Fixed

- 修复编辑领地成员返回按钮异常
- 修复领地购买范围异常

## [0.0.2] - 2024-9-21

### Added

- 新增 12 个事件处理
- 新增依赖 MoreEvents

## [0.0.1] - 2024-9-18

- Initial beta release

<!-- Issue / Pull Request -->

[#211]: https://github.com/IceBlcokMC/PLand/issues/211
[#217]: https://github.com/IceBlcokMC/PLand/issues/217
[#221]: https://github.com/IceBlcokMC/PLand/issues/221

<!-- Release -->

[0.20.3]: https://github.com/IceBlcokMC/PLand/releases/tag/v0.20.3
[0.20.2]: https://github.com/IceBlcokMC/PLand/releases/tag/v0.20.2
[0.0.2]: https://github.com/IceBlcokMC/PLand/releases/tag/v0.0.2
[0.0.1]: https://github.com/IceBlcokMC/PLand/releases/tag/v0.0.1
