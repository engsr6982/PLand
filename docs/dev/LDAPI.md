# LDAPI

?> `LDAPI` 为 PLand 导出的 API  
`LDAPI` 的本质是一个宏，值为 `__declspec(dllimport)`  
头文件中，标记 `LDAPI` 的函数您都能访问、调用

!> ⚠️：您不能直接析构 `LandData_sptr` 以及其它任何由 `PLand` 提供的智能指针，这会导致未知异常。  
您应该从持有此指针的 `class` 处调用对应函数进行释放

!> ⚠️：插件为了方便开发，部分 class 成员声明为 public，但您不应该直接访问、修改这些成员，这会导致未知异常

## class 指南

?> 由于 PLand 提供了整个项目的导出，本文档无法实时更新，请以 SDK 为准

!> 以下表格仅包含您可安全访问的 class，其它 class 虽然被导出，但您不应该访问它们

| 类名             | 描述                                                                          | 备注                   |
| :--------------- | :---------------------------------------------------------------------------- | :--------------------- |
| `PLand`          | PLand 核心类(负责存储、查询)                                                  | -                      |
| `LandData`       | 领地数据 (记录每个领地数据)                                                   | 请使用 `LandData_sptr` |
| `PriceCalculate` | 价格公式解析、计算 ([calculate 计算公式](../md/Config.md#calculate-计算公式)) | -                      |
| `Config`         | 配置文件                                                                      | -                      |
| `EconomySystem`  | 经济系统 (已封装对接双经济)                                                   | -                      |
| `EventListener`  | 事件监听器(监听拦截 Mc 事件)                                                  | -                      |
| `LandPos`        | 坐标基类 (负责处理 JSON 反射)                                                 | -                      |
| `LandAABB`       | 领地坐标 (一个领地的对角坐标)                                                 | -                      |
| `LandSelector`   | 领地选区器(负责圈地、修改范围)                                                | -                      |
| `DrawHandle`     | 绘制管道 (管理领地绘制, 每个玩家都独立分配一个 DrawHandle)                    | -                      |
| `SafeTeleport`   | 安全传送系统                                                                  | -                      |

!> ⚠️：`LandData` 类提供了两个 using, 分别为 `LandData_sptr` 和 `LandData_wptr`。  
`LandData_sptr` 为共享智能指针，原型为 `std::shared_ptr<class LandData>`。  
`LandData_wptr` 为 弱共享智能指针，原型为 `std::weak_ptr<class LandData>`。  
当您需要长期持有 `LandData` 时建议使用 `LandData_wptr` 弱共享智能指针。

## RAII 资源

插件中一些资源采用了 RAII 资源管理，这些资源您不应该构造、析构，它们的生命周期由 **PLand** 的 `ModEntry` 管理。

当您有特殊需要，需要访问这些资源时，您可以使用 `Require<T>` 模板类。

```cpp
#include "pland/Require.h"

void foo() {
    land::Require<land::LandScheduler>()->doSomething();
}
```

`Require<T>` 重载了 `operator->`，它会自动从 PLand 的 ModEntry 中获取 `T` 类型的资源指针并返回。

!> 需要注意的是，当资源没有初始化时，访问它会抛出 `std::runtime_error`，您需要自行捕获。
