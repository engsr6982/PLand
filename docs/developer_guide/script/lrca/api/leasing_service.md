# LeasingService 领地租赁服务类 <Badge type="tip" text="v0.19.0+" />

前缀：`LeasingService_`。

## 检查租赁功能是否启用 <Badge type="tip" text="v0.19.0+" />

`LeasingService_enabled()`

- 参数: 无
- 返回值: 租赁功能是否启用
- 返回值类型: `Boolean`

## 刷新领地调度状态 <Badge type="tip" text="v0.19.0+" />

`LeasingService_refreshSchedule(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: 无（领地不存在则为空操作）
- 返回值类型: `Null`

## 设置领地开始时间 <Badge type="tip" text="v0.19.0+" />

`LeasingService_setStartAt(landId, timestamp)`

- 参数:
  - landId : `Integer`
    领地 ID
  - timestamp : `String`
    时间戳字符串（**秒**）
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 设置领地结束时间 <Badge type="tip" text="v0.19.0+" />

`LeasingService_setEndAt(landId, timestamp)`

- 参数:
  - landId : `Integer`
    领地 ID
  - timestamp : `String`
    时间戳字符串（**秒**）
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 强制冻结领地 <Badge type="tip" text="v0.19.0+" />

`LeasingService_forceFreeze(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 强制回收领地 <Badge type="tip" text="v0.19.0+" />

`LeasingService_forceRecycle(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 追加租赁时长 <Badge type="tip" text="v0.19.0+" />

`LeasingService_addTime(landId, sec)`

- 参数:
  - landId : `Integer`
    领地 ID
  - sec : `Integer`
    要追加的秒数
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 清理过期领地 <Badge type="tip" text="v0.19.0+" />

`LeasingService_cleanExpiredLands(daysOverdue)`

- 参数:
  - daysOverdue : `Integer`
    过期天数阈值
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 转为买断制领地 <Badge type="tip" text="v0.19.0+" />

`LeasingService_toBought(landId)`

- 参数:
  - landId : `Integer`
    领地 ID
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)

## 转为租赁领地 <Badge type="tip" text="v0.19.0+" />

`LeasingService_toLeased(landId, days)`

- 参数:
  - landId : `Integer`
    领地 ID
  - days : `Integer`
    租赁天数
- 返回值: [FFI 协议](/developer_guide/script/lrca/api/types.md#ffiprotocol-协议)
- 返回值类型: `String`(JSON)
