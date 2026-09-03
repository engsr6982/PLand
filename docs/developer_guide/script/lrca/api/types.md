# 类型定义

| C++ 类型                              | 脚本类型           | 说明                               |
| ------------------------------------- | ------------------ | ---------------------------------- |
| `int` / `int64_t`                     | `Integer`          | 整数                               |
| `bool`                                | `Boolean`          | 布尔                               |
| `std::string`                         | `String`           | 字符串                             |
| `std::vector<std::string>`            | `String[]`         | 字符串数组                         |
| `std::vector<int>`                    | `Integer[]`        | 整数数组                           |
| `IntPos`（`std::pair<BlockPos,int>`） | `IntPos`           | 整数坐标对象，含 `x/y/z/dimid`     |
| `FloatPos`（`std::pair<Vec3,int>`）   | `FloatPos`         | 浮点坐标对象，含 `x/y/z/dimid`     |
| `InternalLandAABB`                    | `[IntPos, IntPos]` | 由 `min`、`max` 两个坐标组成的数组 |
| `FfiProtocol` (`Expected<T>`)         | `String`(JSON)     | 接口调用协议，附带结果或异常       |

:::info

- `landId : Integer` — 领地数据库唯一 ID。**领地不存在时**，查询类函数通常返回空值/默认值（空字符串、`-1`、空数组、`false` 等），具体见各函数描述。
- **FfiProtocol 类型函数**：返回值需要 `JSON.parse` 后判断 `ok` 字段；TS 封装层已用 `Expected` 类包装。

:::

## FfiProtocol 协议

Ffi Protocol 协议本质是对 C++ 侧 `Expected<T>` 类的封装，其结构如下：

```ts
/// Expected<> / Expected<T>

interface FfiSuccess<T> {
  ok: true;
  value?: T; // 此字段仅当函数有返回值时存在
}
interface FfiError {
  ok: false;
  error: string;
}

interface FfiProtocol<T> extends FfiSuccess<T> | FfiError {}
```

:::info
当 `ok` 为 `false` 时，`value` 字段为空，`error` 字段包含错误信息。

当 `ok` 为 `true` 时，`error` 字段为空，如果函数具有返回值, `value` 字段包含返回值(反之没有 `value` 字段)。
:::

## 枚举定义

脚本侧枚举一律以**整数**透传（事件回调中的结算 `type`、`landType` 除外，以字符串透传，见 [事件系统](/developer_guide/script/lrca/api/event.md)）。

具体定义以 PLand 源码为准：

- 常规枚举（`LandHoldType`、`LeaseState`、`LandType`、`LandRole`、`LandRecycleReason`）：[`src/pland/enums/`](https://github.com/IceBlcokMC/PLand/blob/main/src/pland/enums/)
- 范围调整结算类型 `LandResizeSettlement::Type`：[`src/pland/land/LandResizeSettlement.h`](https://github.com/IceBlcokMC/PLand/blob/main/src/pland/land/LandResizeSettlement.h)

## 跨版本兼容性

| 标记                                        | 说明                                              |
| ------------------------------------------- | ------------------------------------------------- |
| <Badge type="tip" text="v0.19.0+" />        | 仅 v0.19.0 及以后版本可用(其它版本定义以此类推)。 |
| <Badge type="warning" text="deprecated" />  | 已废弃，不建议使用(接口可能在将来被移除)。        |
| <Badge type="warning" text="performance" /> | 此接口可能影响性能，请谨慎使用。                  |

:::warning 对于标记废弃的接口，PLand 可能会在未来版本中移除它，请尽快迁移到新的接口。
:::
