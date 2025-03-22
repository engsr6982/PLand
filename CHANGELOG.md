# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.7.0]

### Added

- 新增 `en_US`、`ru_RU` 语言文件 #17
- 新增 `/pland list op` 命令 #15
- 新增 `/pland set language` 命令 #19
- 新增 `/pland this` 命令
- `Config` 添加 `subLand` 子领地相配置 #18

### Changed

- 移除 `Particle`、`LandDraw`
- 优化 `PLand::getLandAt` 查询
- 重构 `Calculate` 为 `PriceCalculate`
- 重构部分代码

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
