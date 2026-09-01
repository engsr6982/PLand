# PLand-NAPI <Badge type="warning" text="实验性" />

::: warning 实验性
PLand-NAPI 目前处于**实验性**阶段，**只是一个理想的推荐方案**，还有一些问题没有验证和解决。

- API 可能随版本调整，请关注更新日志
- 生产环境建议使用稳定可用的 [PLand-LegacyRemoteCallApi](/developer_guide/script/LegacyRemoteCallApi)
:::

::: danger 已知问题
- 相关问题请查看 [issues](https://github.com/IceBlcokMC/PLand-NAPI/issues)
:::

PLand 的**原生绑定 API**（Native Bind API），用于在 **LegacyScriptEngine（quickjs / nodejs 后端）** 中直接调用 PLand 的 C++ 接口。

- 脚本可直接**持有**领地对象，性能接近原生调用
- 提供完整的 TypeScript 类型定义，开发体验好
- 支持事件订阅（EventBus）

> 项目地址：[github.com/IceBlcokMC/PLand-NAPI](https://github.com/IceBlcokMC/PLand-NAPI)

## 安装

::: warning 重要
PLand-NAPI **必须与 PLand 一起安装**，且 **minor 版本必须一致**（例如 PLand 0.21.x 对应 PLand-NAPI 0.21.x），否则无法使用。
:::

### Lip 安装

```bash
# 默认（quickjs 后端）
lip install github.com/IceBlcokMC/PLand-NAPI

# 或手动指定后端
lip install github.com/IceBlcokMC/PLand-NAPI#quickjs
lip install github.com/IceBlcokMC/PLand-NAPI#nodejs
```

### 手动安装

1. 前往 [Releases](https://github.com/IceBlcokMC/PLand-NAPI/releases) 下载对应后端类型的安装包
2. 解压并将 `pland-napi-<backend>` 文件夹放入 `plugins` 文件夹

## 导入方式

两个后端的导入方式不同：

### quickjs 后端

```js
import {} from /* 你的插件路径 */ "pland-napi-quickjs.dll";
```

### nodejs 后端

> Node.js 暂不支持使用 `import` 语法导入 C++ 插件（Node Addon），需改用 `require`：

```js
const {
  /* ... */
} = require("../pland-napi-nodejs/pland-napi-nodejs.node");
```

## 快速示例

### 创建并操作一个领地对象

```js
import {
  Land,
  LandAABB,
  LandType,
  LandHoldType,
  LeaseState,
  LandRole,
} from "pland-napi-quickjs.dll";

// 创建一个领地对象（范围 + 维度 + 是否3D + 主人 UUID）
const aabb = new LandAABB({ x: 0, y: 0, z: 0 }, { x: 100, y: 64, z: 100 });
const land = new Land(aabb, 0, true, "你的玩家UUID");

// 读取属性
console.log(land.id);          // 领地 ID（未注册时为 -1n）
console.log(land.is3D);        // 是否 3D 领地
console.log(land.type);        // LandType.Ordinary
console.log(land.holdType);    // LandHoldType.Bought
console.log(land.leaseState);  // LeaseState.None

// 成员管理
land.addLandMember("成员UUID");
console.log(land.isMember("成员UUID")); // true
console.log(land.getPermType("成员UUID")); // LandRole.Member
```

### 修改领地权限

```js
// 每次 getPermTable() 返回一个新副本，修改后需写回
const pt = land.getPermTable();
pt.environment.allowFireSpread = false;           // 禁止火焰蔓延
pt.role.allowDestroy = { member: true, actor: false }; // 成员可破坏，访客不可
land.setPermTable(pt); // 写回
```

### 订阅领地事件

```js
import { EventBus } from "pland-napi-quickjs.dll";

// 订阅事件，返回监听器 ID
const id = EventBus.subscribe("ConfigReloadEvent", (event) => {
  console.log("配置已重载:", event.id);
});

// 取消订阅
EventBus.unsubscribe(id);
```

所有可用事件见类型定义中的 `EventMap`（如 `LandRecycleEvent`、`LandResizedEvent`、`MemberChangedEvent`、`OwnerChangedEvent` 等）。

## 类型定义

PLand-NAPI 随包发布完整的 **TypeScript 类型定义**（`.d.ts`），位于仓库的 [`types`](https://github.com/IceBlcokMC/PLand-NAPI/tree/main/types) 文件夹：

| 文件 | 内容 |
|:----|:----|
| `Global.d.ts` | 基础类型别名（`player_t`、`vector3_t`、`BlockPos` 等） |
| `Enums.d.ts` | 枚举（`LandType`、`LandHoldType`、`LeaseState`、`LandRole` 等） |
| `Land.d.ts` | `Land` 类定义 |
| `LandAABB.d.ts` | `LandAABB` 类定义 |
| `PermTable.d.ts` | 权限表定义 |
| `Event.d.ts` | 事件系统定义 |

类型文件也会随 [Releases](https://github.com/IceBlcokMC/PLand-NAPI/releases) 一起发布，提供完整的编辑提示。

## 许可证

PLand-NAPI 使用 [LGPL-3.0 or later](https://github.com/IceBlcokMC/PLand-NAPI/blob/main/LICENSE) 许可证。

> 注意: PLand-NAPI 所使用的第三方库均遵循其各自的许可证
