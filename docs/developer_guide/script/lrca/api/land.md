# Land 领地对象

前缀：`Land_`。除特殊标注外，第一参数均为 `landId`。

## 判断领地是否为系统所有 <Badge type="tip" text="v0.19.0+" />

`Land_isSystemOwned(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为系统所有
- 返回值类型: `Boolean`

## 判断领地是否为买断制领地 <Badge type="tip" text="v0.19.0+" />

`Land_isBought(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为买断制领地
- 返回值类型: `Boolean`

## 判断领地是否已被租赁 <Badge type="tip" text="v0.19.0+" />

`Land_isLeased(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否已被租赁
- 返回值类型: `Boolean`

## 判断租赁是否有效 <Badge type="tip" text="v0.19.0+" />

`Land_isLeaseActive(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 租赁是否正常
- 返回值类型: `Boolean`

## 判断租赁是否被冻结 <Badge type="tip" text="v0.19.0+" />

`Land_isLeaseFrozen(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 租赁是否被冻结
- 返回值类型: `Boolean`

## 判断租赁是否已过期 <Badge type="tip" text="v0.19.0+" />

`Land_isLeaseExpired(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 租赁是否已过期
- 返回值类型: `Boolean`

## 获取领地持有类型 <Badge type="tip" text="v0.19.0+" />

`Land_getHoldType(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 领地持有类型：`0`=买断 / `1`=租赁
- 返回值类型: `Integer`

## 获取租赁状态 <Badge type="tip" text="v0.19.0+" />

`Land_getLeaseState(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 租赁状态：`0`=无 / `1`=正常 / `2`=冻结 / `3`=到期
- 返回值类型: `Integer`

## 获取租赁开始时间 <Badge type="tip" text="v0.19.0+" />

`Land_getLeaseStartAt(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 租赁开始时间戳（**秒**，字符串形式；领地不存在返回空字符串）
- 返回值类型: `String`

## 获取租赁结束时间 <Badge type="tip" text="v0.19.0+" />

`Land_getLeaseEndAt(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 租赁结束时间戳（**秒**，字符串形式；领地不存在返回空字符串）
- 返回值类型: `String`

## 获取领地 AABB 范围

`Land_getAABB(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 领地范围 `[min, max]`，两个元素均为 `IntPos`；领地不存在返回空数组
- 返回值类型: `[IntPos, IntPos]`

## 获取领地传送坐标

`Land_getTeleportPos(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 传送坐标 `IntPos`（含 `dimid`）
- 返回值类型: `IntPos`

## 设置领地传送坐标

`Land_setTeleportPos(landId, pos)`

- 参数:
  - landId : `Integer`
    领地 ID
  - pos : `IntPos`
    传送坐标（仅使用 `x/y/z`，`dimid` 会被忽略）
- 返回值: 无
- 返回值类型: `Null`

## 获取领地 ID

`Land_getId(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 领地 ID（领地不存在返回 `-1`）
- 返回值类型: `Integer`

## 获取领地所在维度 ID

`Land_getDimensionId(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 维度 ID（领地不存在返回 `-1`）
- 返回值类型: `Integer`

## 获取权限表

`Land_getPermTable(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 权限表 JSON 字符串（领地不存在返回空字符串），可用 `JSON.parse` 解析
- 返回值类型: `String`

## 设置权限表

`Land_setPermTable(landId, permTable)`

- 参数:
  - landId : `Integer`
    领地 ID
  - permTable : `String`
    权限表 JSON 字符串（结构同 `Land_getPermTable` 返回值）
- 返回值: 无
- 返回值类型: `Null`

## 获取领地主人

`Land_getOwner(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 领地主人 UUID（领地不存在返回空字符串）
- 返回值类型: `String`

## 修改领地主人

`Land_setOwner(landId, owner)`

- 参数:
  - landId : `Integer`
    领地 ID
  - owner : `String`
    新的领地主人 UUID
- 返回值: 是否设置成功（领地不存在或 UUID 无效返回 `false`）
- 返回值类型: `Boolean`

## 获取原始主人标识 <Badge type="warning" text="deprecated" />

`Land_getRawOwner(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 原始存储的主人标识（可能为 XUID 或 UUID；领地不存在返回空字符串）
- 返回值类型: `String`

:::warning 此接口由于历史遗留问题导致 Owner 字段不一定为 UUID, 领地内部会处理 XUID 转换，通常无需特别处理，建议改用 `Land_getOwner()`。
:::

## 获取领地成员列表

`Land_getMembers(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 成员 UUID 列表
- 返回值类型: `String[]`

## 添加领地成员

`Land_addLandMember(landId, member)`

- 参数:
  - landId : `Integer`
    领地 ID
  - member : `String`
    要添加的成员 UUID
- 返回值: 是否添加成功（此函数拒绝将 Owner 添加为 Member；UUID 无效返回 `false`）
- 返回值类型: `Boolean`

## 移除领地成员

`Land_removeLandMember(landId, member)`

- 参数:
  - landId : `Integer`
    领地 ID
  - member : `String`
    要移除的成员 UUID
- 返回值: 是否移除成功
- 返回值类型: `Boolean`

## 获取领地名称

`Land_getName(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 领地名称
- 返回值类型: `String`

## 设置领地名称

`Land_setName(landId, name)`

- 参数:
  - landId : `Integer`
    领地 ID
  - name : `String`
    新的领地名称
- 返回值: 无
- 返回值类型: `Null`

## 获取领地原始购买价格

`Land_getOriginalBuyPrice(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 领地原始购买价格
- 返回值类型: `Integer`

## 设置领地原始购买价格

`Land_setOriginalBuyPrice(landId, originalBuyPrice)`

- 参数:
  - landId : `Integer`
    领地 ID
  - originalBuyPrice : `Integer`
    要设置的原始购买价格
- 返回值: 无
- 返回值类型: `Null`

## 判断领地是否为 3D 领地

`Land_is3D(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为 3D 领地
- 返回值类型: `Boolean`

## 判断指定 UUID 是否为领地主人

`Land_isOwner(landId, uuid)`

- 参数:
  - landId : `Integer`
    领地 ID
  - uuid : `String`
    玩家 UUID
- 返回值: 是否为领地主人（UUID 无效返回 `false`）
- 返回值类型: `Boolean`

## 判断指定 UUID 是否为领地成员

`Land_isMember(landId, uuid)`

- 参数:
  - landId : `Integer`
    领地 ID
  - uuid : `String`
    玩家 UUID
- 返回值: 是否为领地成员（UUID 无效返回 `false`）
- 返回值类型: `Boolean`

## 判断是否为转换领地 <Badge type="warning" text="deprecated" />

`Land_isConvertedLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为转换领地
- 返回值类型: `Boolean`

> **废弃提示：** 该接口可能在未来移除。

## 判断主人原始数据是否为 XUID <Badge type="warning" text="deprecated" />

`Land_isOwnerDataIsXUID(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 主人原始数据是否为 XUID
- 返回值类型: `Boolean`

> **废弃提示：** 该接口可能在未来移除。

## 判断与坐标点（带半径）是否碰撞

`Land_isCollision(landId, pos, radius)`

- 参数:
  - landId : `Integer`
    领地 ID
  - pos : `IntPos`
    中心坐标
  - radius : `Integer`
    半径
- 返回值: 领地的 AABB 与「以 pos 为中心、radius 半径」的范围是否碰撞
- 返回值类型: `Boolean`

## 判断与坐标点是否碰撞

`Land_isCollision2(landId, a, b)`

- 参数:
  - landId : `Integer`
    领地 ID
  - a : `IntPos`
    方盒对角点 A
  - b : `IntPos`
    方盒对角点 B
- 返回值: 领地的 AABB 与「以 `a`、`b` 为对角点构成的方盒」是否碰撞
- 返回值类型: `Boolean`

## 判断数据是否已被修改

`Land_isDirty(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 数据是否被修改（调用任意 `set` 方法后为 `true`，保存后复位为 `false`）
- 返回值类型: `Boolean`

## 获取领地类型

`Land_getType(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 领地类型：`0`=普通 / `1`=父 / `2`=混合 / `3`=子（领地不存在返回 `-1`）
- 返回值类型: `Integer`

## 判断是否有父领地

`Land_hasParentLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否有父领地
- 返回值类型: `Boolean`

## 判断是否有子领地

`Land_hasSubLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否有子领地
- 返回值类型: `Boolean`

## 判断是否为子领地

`Land_isSubLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为子领地（有父领地、无子领地）
- 返回值类型: `Boolean`

## 判断是否为父领地

`Land_isParentLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为父领地（有子领地、无父领地）
- 返回值类型: `Boolean`

## 判断是否为混合领地

`Land_isMixLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为混合领地（有父领地、有子领地）
- 返回值类型: `Boolean`

## 判断是否为普通领地

`Land_isOrdinaryLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否为普通领地（无父领地、无子领地）
- 返回值类型: `Boolean`

## 判断是否可以创建子领地

`Land_canCreateSubLand(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 是否满足嵌套层级限制、可以创建子领地
- 返回值类型: `Boolean`

## 获取父领地 ID

`Land_getParentLandID(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 父领地 ID（领地不存在返回 `-1`）
- 返回值类型: `Integer`

## 获取子领地 ID 列表

`Land_getSubLandIDs(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 当前领地名下的所有子领地 ID 列表
- 返回值类型: `Integer[]`

## 获取嵌套层级

`Land_getNestedLevel(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 嵌套层级（相对于父领地）
- 返回值类型: `Integer`

## 获取玩家在领地的权限类别 <Badge type="warning" text="deprecated v0.22.1" />

`Land_getPermType(landId, uuid)`

- 参数:
  - landId : `Integer`
    领地 ID
  - uuid : `String`
    玩家 UUID
- 返回值: 权限类别：`0`=管理员 / `1`=主人 / `2`=成员 / `3`=实体（领地或 UUID 无效返回 `-1`）
- 返回值类型: `Integer`

## 获取玩家在当前领地的有效角色 <Badge type="tip" text="v0.22.1+" />

`Land_getEffectiveRole(landId, uuid)`

- 参数:
  - landId : `Integer`
    领地 ID
  - uuid : `String`
    玩家 UUID
- 返回值: 权限类别：`0`=管理员 / `1`=主人 / `2`=成员 / `3`=实体（领地或 UUID 无效返回 `-1`）
- 返回值类型: `Integer`

:::tip 领地冻结时返回 Actor，否则根据权限返回对应角色
:::
