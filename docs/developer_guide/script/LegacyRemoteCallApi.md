# PLand-LegacyRemoteCallApi

PLand 的 **LegacyRemoteCallApi（LRCA）** 实现，用于在 **LegacyScriptEngine-QuickJs/NodeJs** 中调用 PLand 的 API。

> 项目地址：[github.com/IceBlcokMC/PLand-LegacyRemoteCallApi](https://github.com/IceBlcokMC/PLand-LegacyRemoteCallApi)

::: warning 注意事项
- 本项目仅对 PLand C++ API 进行封装，**不包含 PLand 的任何代码**，请确保已经安装了 PLand
- 由于引擎限制，无法做到原生持有 Native 对象，因此采用了一些折衷方案（句柄），**对于大型项目存在性能问题**，建议大型项目使用 [C++ API](/developer_guide/QuickStart)
:::

## 安装

::: tip 需与 PLand 一起安装。
:::

```bash
lip install github.com/IceBlcokMC/PLand-LegacyRemoteCallApi
```

## 导入方式

从 **0.8.0** 版本开始，同时提供 **ESM** 和 **CJS** 两种导出方式，你可以根据自己的需求选择。

::: warning 关于 quickjs ESM 的路径解析（引擎 Bug）
LSE 的 quickjs 后端在加载 **入口模块**（`manifest.json` 中指定的入口文件）时，相对 `import` 路径是相对于 **服务器工作目录（CWD）** 解析的，而不是脚本文件所在目录。

也就是说，在入口文件中写 `import ... from "./xxx.js"` 会指向 `服务器根目录/xxx.js`（通常不存在），导致加载失败。

**应对方式**：参考官方 [LanguageSupport 文档](https://lse.levimc.org/zh/apis/LanguageSupport/#关于-quickjs-esmodule-的路径解析行为)，使用 **bootstraps.js 引导文件**规避：

1. 在 `manifest.json` 中把入口设为 `bootstraps.js`
2. `bootstraps.js` 中使用**相对于服务器根目录**的路径导入实际入口文件
3. 实际入口文件（非入口模块）内部的 `import` 遵循标准 ESM 规则，可用正常相对路径
:::

### 推荐：bootstraps.js 引导模式

假设你的插件位于 `plugins/my_plugin/`，实际入口为 `Test.js`：

```js
// bootstraps.js（manifest.json 的入口，位于 plugins/my_plugin/）
// 入口模块的 import.meta.url 为服务器 CWD，所以使用相对于服务器根目录的路径
import * as _ from "./plugins/my_plugin/Test.js";
```

```js
// Test.js（实际入口，非入口模块，遵循标准 ESM 规则）
import { LandRegistry } from "../PLand-LegacyRemoteCallApi/lib/esm/imports/LandRegistry.js";
import { Land }         from "../PLand-LegacyRemoteCallApi/lib/esm/imports/Land.js";
import { LDEvent }      from "../PLand-LegacyRemoteCallApi/lib/esm/imports/LDEvents.js";
```

### 直接在入口模块中导入

如果你不想用引导文件，在入口模块中必须使用**相对于服务器根目录**的完整路径：

```js
// 入口模块（相对于服务器 CWD 解析）
import { LandRegistry } from "./plugins/PLand-LegacyRemoteCallApi/lib/esm/imports/LandRegistry.js";
```

### CJS（nodejs 后端）

```js
const { LandRegistry } = require("./plugins/PLand-LegacyRemoteCallApi/lib/cjs/imports/LandRegistry.js");
```

## 快速示例

::: warning 调用时机
由于 PLand 内部资源初始化顺序问题，对脚本的 LRCA 导出会在 **PLand 初始化后**导出。作为脚本方，**不能在全局作用域、立即执行函数内调用领地 API**，请在服务器启动后（`onServerStarted`）调用。
:::

### 查询领地

```js
// 假设这是你的实际入口文件（非入口模块，遵循标准 ESM 规则）
import { LandRegistry } from "../PLand-LegacyRemoteCallApi/lib/esm/imports/LandRegistry.js";
import { Land }         from "../PLand-LegacyRemoteCallApi/lib/esm/imports/Land.js";

mc.listen("onServerStarted", () => {
    // 获取单个领地（未找到返回 null）
    const land = LandRegistry.getLand(2);

    // 获取所有领地
    const lands = LandRegistry.getLands();

    // 按维度 / 按玩家查询
    LandRegistry.getLands(0);            // 主世界所有领地
    LandRegistry.getLands(playerUuid, true); // 某玩家的领地

    // 按坐标查询
    const landAt = LandRegistry.getLandAt(new IntPos(100, 64, 200, 0));
});
```

### 操作领地对象

LRCA 的 `Land` 类实际是一个**句柄**，只要知道领地 ID 就可以直接构造，无需查询：

```js
// 直接通过 ID 构造领地句柄
const land = new Land(2);

// 读取领地信息
console.log(land.getLeaseState());   // 租赁状态
console.log(land.getLeaseEndAt());   // 租赁结束时间
console.log(land.getAABB());         // 领地范围
console.log(land.isBought());        // 是否买断
```

::: warning 性能提示
脚本尽可能**不要长期持有**领地对象，因为 LRCA 提供的封装都是句柄。即使持有了，底层在玩家删除领地时也会释放领地指针。
:::

### 监听领地事件

```js
import { LDEvent } from "../PLand-LegacyRemoteCallApi/lib/esm/imports/LDEvents.js";

// 注册事件监听器
// 注意: LDEvent 级所有相关 API，在 onLoad 阶段调用会报错，因为此时 API 还未导出、PLand 未初始化完成
function setupListeners() {
    LDEvent.listen("PlayerEnterLandEvent", (player, landId) => {
        console.log(`${player.name} 进入了领地 ${landId}`);
    });
}

mc.listen("onServerStarted", () => {
    setupListeners();
});
```

## 主要 API

| 模块 | 说明 |
|:----|:----|
| `LandRegistry` | 领地注册表查询与操作（`getLand`、`getLands`、`getLandAt`、`isOperator`、`createSnapshot` 等） |
| `Land` | 领地句柄（`getAABB`、`getOwner`、`getMembers`、租赁状态等） |
| `LandAABB` | 领地范围工具类 |
| `LeasingService` | 租赁服务（`forceFreeze`、`forceRecycle`、`toBought`、`toLeased` 等） |
| `LDEvent` | 事件监听（`PlayerEnterLandEvent`、`PlayerBuyLandEvent`、`OwnerChangedEvent` 等） |

::: tip 更多示例
仓库 `test` 文件夹下提供了完整的测试代码（`test/Test.js`），可作为参考。
:::
