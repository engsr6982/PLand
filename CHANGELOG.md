# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
## [Unreleased]

## [0.8.2] - 2025-06-05

### 🧩 权限与逻辑优化

- 将生物的类别修改为配置文件可自定义的列表，不再依赖family来判断生物类别 #84 @yangyangzhong82



## [Unreleased]

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

- 修复 `ActorHurtEvent` `ActorRideEvent` `FarmDecayEvent` `MobHurtEffectEvent` `PressurePlateTriggerEvent` `ProjectileSpawnEvent` `RedstoneUpdateEvent` 事件意外拦截领地外事件。

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
