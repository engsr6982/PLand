# LandRegistry 领地注册表对象

前缀：`LandRegistry_`。

## 创建数据库快照 <Badge type="tip" text="v0.19.0+" />

`LandRegistry_createSnapshot(dirName)`

- 参数:
  - dirName : `String`
    快照文件夹名称，传空字符串则使用当前时间戳
- 返回值: 无
- 返回值类型: `Null`

:::info 快照写入磁盘 `snapshots/<dirName ?? 时间戳>`。  
快照为**异步**任务，未完成时目录下存在 `.incomplete` 文件
:::

## 判断是否为领地操作员

`LandRegistry_isOperator(uuid)`

- 参数:
  - uuid : `String`
    玩家 UUID
- 返回值: 是否为领地操作员（UUID 无效返回 `false`）
- 返回值类型: `Boolean`

## 添加领地操作员

`LandRegistry_addOperator(uuid)`

- 参数:
  - uuid : `String`
    玩家 UUID
- 返回值: 是否添加成功（UUID 无效返回 `false`）
- 返回值类型: `Boolean`

## 移除领地操作员

`LandRegistry_removeOperator(uuid)`

- 参数:
  - uuid : `String`
    玩家 UUID
- 返回值: 是否移除成功（UUID 无效返回 `false`）
- 返回值类型: `Boolean`

## 获取全部领地操作员

`LandRegistry_getOperators()`

- 参数: 无
- 返回值: 全部领地操作员 UUID 列表
- 返回值类型: `String[]`

## 获取（或创建）玩家设置

`LandRegistry_getOrCreatePlayerSettings(uuid)`

- 参数:
  - uuid : `String`
    玩家 UUID
- 返回值: 玩家设置 JSON 字符串（UUID 无效返回空字符串）
- 返回值类型: `String`

## 判断是否已存在指定领地

`LandRegistry_hasLand(id)`

- 参数:
  - id : `Integer`
    领地 ID
- 返回值: 是否已存在该 ID 的领地
- 返回值类型: `Boolean`

## 创建并添加普通领地

`LandRegistry_addOrdinaryLand(aabb, is3D, owner)`

- 参数:
  - aabb : `[IntPos, IntPos]`
    领地范围 `[min, max]`（两个坐标必须在同一维度，维度取 min 的 `dimid`）
  - is3D : `Boolean`
    是否为 3D 领地
  - owner : `String`
    领地主人 UUID
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 获取领地

`LandRegistry_getLand(id)`

- 参数:
  - id : `Integer`
    领地 ID
- 返回值: 领地 ID（领地不存在返回 `-1`）
- 返回值类型: `Integer`

## 移除普通领地

`LandRegistry_removeOrdinaryLand(id)`

- 参数:
  - id : `Integer`
    领地 ID
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 获取所有领地 <Badge type="warning" text="performance" />

`LandRegistry_getLands()`

- 参数: 无
- 返回值: 所有领地的 ID 列表
- 返回值类型: `Integer[]`

:::warning 此接口会在每次调用时遍历所有领地获取id集合，如果领地数量较多，可能造成性能问题。
:::

## 获取指定维度的领地

`LandRegistry_getLands1(dimid)`

- 参数:
  - dimid : `Integer`
    维度 ID
- 返回值: 该维度的所有领地 ID 列表
- 返回值类型: `Integer[]`

## 获取玩家名下的领地

`LandRegistry_getLands2(uuid, includeShared)`

- 参数:
  - uuid : `String`
    玩家 UUID
  - includeShared : `Boolean`
    是否包含其它玩家共享的领地
- 返回值: 玩家名下的领地 ID 列表（UUID 无效返回空数组）
- 返回值类型: `Integer[]`

## 获取玩家在指定维度的领地

`LandRegistry_getLands3(uuid, dimid)`

- 参数:
  - uuid : `String`
    玩家 UUID
  - dimid : `Integer`
    维度 ID
- 返回值: 玩家在该维度的领地 ID 列表（UUID 无效返回空数组）
- 返回值类型: `Integer[]`

## 批量获取领地

`LandRegistry_getLands4(lds)`

- 参数:
  - lds : `Integer[]`
    领地 ID 列表
- 返回值: 与传入 ID 对应的领地 ID 列表
- 返回值类型: `Integer[]`

## 获取玩家在领地的权限类别 <Badge type="warning" text="deprecated v0.22.1" />

`LandRegistry_getPermType(uuid, landID, includeOperator)`

- 参数:
  - uuid : `String`
    玩家 UUID
  - landID : `Integer`
    领地 ID
  - includeOperator : `Boolean`
    是否将「领地操作员」判定计为管理员权限
- 返回值: 权限类别：`0`=管理员 / `1`=主人 / `2`=成员 / `3`=实体
- 返回值类型: `Integer`

## 获取玩家在当前领地的有效角色 <Badge type="tip" text="v0.22.1+" />

`LandRegistry_getEffectiveRole(uuid, landID, includeOperator)`

- 参数:
  - uuid : `String`
    玩家 UUID
  - landID : `Integer`
    领地 ID
  - includeOperator : `Boolean`
    是否将「领地操作员」判定计为管理员权限
- 返回值: 权限类别：`0`=管理员 / `1`=主人 / `2`=成员 / `3`=实体
- 返回值类型: `Integer`

:::tip 领地冻结时返回 Actor，否则根据权限返回对应角色
:::

## 获取指定坐标所在的领地

`LandRegistry_getLandAt(pos)`

- 参数:
  - pos : `IntPos`
    坐标（含 `dimid`）
- 返回值: 该坐标所在领地的 ID（无领地返回 `-1`）
- 返回值类型: `Integer`

## 获取指定坐标半径内的领地

`LandRegistry_getLandAt1(pos, radius)`

- 参数:
  - pos : `IntPos`
    中心坐标（含 `dimid`）
  - radius : `Integer`
    半径
- 返回值: 半径内覆盖的所有领地 ID 列表
- 返回值类型: `Integer[]`

## 获取 AABB 区域内的领地

`LandRegistry_getLandAt2(a, b)`

- 参数:
  - a : `IntPos`
    区域 min 坐标（含 `dimid`）
  - b : `IntPos`
    区域 max 坐标
- 返回值: 该 AABB 区域内覆盖的所有领地 ID 列表
- 返回值类型: `Integer[]`

## 刷新领地范围缓存

`LandRegistry_refreshLandRange(id)`

- 参数:
  - id : `Integer`
    领地 ID
- 返回值: 无
- 返回值类型: `Null`
