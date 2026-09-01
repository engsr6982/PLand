# LDAPI

::: tip `LDAPI` 为 PLand 导出的 API  
`LDAPI` 的本质是一个宏，值为 `__declspec(dllimport)`  
头文件中，标记 `LDAPI` 的函数您都能访问、调用
:::

::: tip 由于 PLand 尚未进入 v1.0.0 内部API变化频繁，本文档无法实时更新，请以 SDK 为准
:::

## class 指南

所有公开 API 均在 `land` 命名空间下（事件在 `land::event`，服务在 `land::service`）。

| 类名                   | 命名空间        | 描述                                                                                |
| :--------------------- | :-------------- | :---------------------------------------------------------------------------------- |
| `PLand`                | `land`          | 插件主入口（单例），从这里获取注册表、服务等                                        |
| `LandRegistry`         | `land`          | 领地注册表（存储、查询的核心类）                                                    |
| `Land`                 | `land`          | 领地代理类（提供对原始数据的封装 API）                                              |
| `LandAABB` / `LandPos` | `land`          | 领地范围 / 坐标基类                                                                 |
| `PriceCalculate`       | `land`          | 价格公式解析、计算 ([formula 价格表达式](/user_guide/Config.md#formula-价格表达式)) |
| `ConfigProvider`       | `land`          | 配置文件读取                                                                        |
| `EconomySystem`        | `land`          | 经济系统 (已封装对接双经济)                                                         |
| `ServiceLocator`       | `land::service` | 服务定位器（获取各类 Service）                                                      |
| `LandSelector`         | `land`          | 领地选区器(负责圈地、修改范围)                                                      |
| `SelectorManager`      | `land`          | 选区管理器                                                                          |
| `DrawHandleManager`    | `land`          | 绘制管道管理 (每个玩家独立分配)                                                     |

::: warning ⚠️：当您需要长期持有 `Land` 时建议使用 `std::weak_ptr<Land>` 弱引用。
:::

## 常用 API 示例

以下示例展示各核心类的常见用法，完整签名请以 SDK 头文件为准。

### 获取插件主入口

```cpp
#include "pland/PLand.h"

// PLand 是单例，通过 getInstance() 获取
auto& pland = land::PLand::getInstance();

// 从主入口获取各种子系统
auto& registry  = pland.getLandRegistry();       // 领地注册表
auto& services  = pland.getServiceLocator();     // 服务定位器
```

### 查询领地（LandRegistry）

```cpp
#include "pland/PLand.h"
#include "pland/land/repo/LandRegistry.h"

auto& registry = land::PLand::getInstance().getLandRegistry();

// 按坐标查询（BlockPos + 维度 ID）
auto land = registry.getLandAt(BlockPos{100, 64, 200}, 0);
if (land) {
    // 找到了领地
}

// 按领地 ID 查询
auto land2 = registry.getLand(123);

// 查询某个玩家的所有领地
auto lands = registry.getLands(playerUuid);

// 查询某维度下所有领地
auto lands2 = registry.getLands(0);

// 判断某个玩家是否为管理员
if (registry.isOperator(playerUuid)) { /* ... */ }
```

### 读取领地信息（Land）

```cpp
#include "pland/land/Land.h"

// land 是 std::shared_ptr<Land>
std::string name  = land->getName();        // 领地名
auto        aabb  = land->getAABB();        // 领地范围
auto        owner = land->getOwner();       // 主人 UUID
auto        dimid = land->getDimensionId(); // 维度 ID
bool        is3D  = land->is3D();           // 是否 3D 领地
bool        isSub = land->isSubLand();      // 是否子领地

// 玩家在此领地的权限类别（LandRole: Admin/Owner/Member/Actor）
auto role = land->getPermType(playerUuid);
```

### 领地操作（通过 Service）

::: tip 创建、删除、转让等操作请走 `LandManagementService`，它会处理校验、价格、事件等完整流程，不要直接操作 `LandRegistry`。
:::

```cpp
#include "pland/PLand.h"
#include "pland/service/ServiceLocator.h"
#include "pland/service/LandManagementService.h"

auto& mgmtService = land::PLand::getInstance().getServiceLocator().getLandManagementService();

// 给领地添加成员（返回 ll::Expected，可通过 has_value() 判断成功）
auto result = mgmtService.addMember(player, land, targetUuid);
if (result.has_value()) {
    // 添加成功
}

// 转让领地
mgmtService.transferLand(player, land, newOwner);
```

### 价格计算

```cpp
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/service/ServiceLocator.h"
#include "pland/service/LandPriceService.h"

auto& priceService = land::PLand::getInstance().getServiceLocator().getLandPriceService();

// 计算一块领地的购买价格
auto aabb = land::LandAABB::make(LandPos{0, 0, 0}, LandPos{100, 64, 100});
auto price = priceService.getOrdinaryLandPrice(aabb, 0, /* is3D */ true);
if (price.has_value()) {
    int64_t original    = price->mOriginalPrice;    // 原价
    int64_t discounted  = price->mDiscountedPrice;  // 折扣后价格
}
```

### 经济系统

```cpp
#include "pland/economy/EconomySystem.h"

auto& economy = land::EconomySystem::getInstance();

// 校验余额并扣款（返回 false 表示余额不足）
bool ok = economy.reduceChecked(playerUuid, /* 金额 */ 1000);
```

### 事件监听

事件系统使用 LeviLamina 的 `EventBus`，详见 [Event](/developer_guide/Event)：

```cpp
#include "pland/events/domain/OwnerChangedEvent.h"
#include "ll/api/event/EventBus.h"

ll::event::EventBus::getInstance().emplaceListener<land::event::OwnerChangedEvent>(
    [](land::event::OwnerChangedEvent const& ev) {
        auto oldOwner = ev.oldOwner(); // 旧主人
        auto newOwner = ev.newOwner(); // 新主人
    }
);
```

## RAII 资源

插件中许多资源采用了 RAII 机制，它们由 `PLand` 主入口类管理。  
当您有需要时，可以从 `PLand` 获取它们

## `internal` 相关

`internal` 命名空间 / 文件夹 下的文件、方法、API 为内部 API  
PLand 默认不导出这些API的符号，也不推荐访问、修改 `internal` 下的内容

## `service` 相关

> v0.18+ 版本开始，PLand 重构了许多代码，引入了 service 模式

您可以通过 `PLand::getInstance().getServiceLocator()` 获取 `ServiceLocator`

所有 `Service` 都受 `ServiceLocator` 管理，您可以通过 `ServiceLocator` 获取它们：

| Service                 | 作用                                 |
| :---------------------- | :----------------------------------- |
| `LandManagementService` | 领地管理（创建、删除、转让、成员等） |
| `LandHierarchyService`  | 子领地层级管理                       |
| `LandPriceService`      | 价格计算                             |
| `LeasingService`        | 租赁管理                             |
| `SelectionService`      | 选区任务管理                         |
