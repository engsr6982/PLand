# LDAPI

?> `LDAPI` 为 PLand 导出的 API  
`LDAPI` 的本质是一个宏，值为 `__declspec(dllimport)`  
头文件中，标记 `LDAPI` 的函数您都能访问、调用

!> ⚠️：您不能直接析构 `SharedLand` 以及其它任何由 `PLand` 提供的智能指针，这会导致未知异常。  
您应该从持有此指针的 `class` 处调用对应函数进行释放

!> ⚠️：插件为了方便开发，部分 class 成员声明为 public，但您不应该直接访问、修改这些成员，这会导致未知异常

## class 指南

?> 由于 PLand 提供了整个项目的导出，本文档无法实时更新，请以 SDK 为准

!> 以下表格仅包含您可安全访问的 class，其它 class 虽然被导出，但您不应该访问它们

| 类名             | 描述                                                                          | 备注                |
| :--------------- | :---------------------------------------------------------------------------- | :------------------ |
| `LandRegistry`   | LandRegistry 核心类(负责存储、查询)                                           | -                   |
| `Land`           | 领地代理类(提供对原始数据的封装 API)                                          | 请使用 `SharedLand` |
| `PriceCalculate` | 价格公式解析、计算 ([calculate 计算公式](../md/Config.md#calculate-计算公式)) | -                   |
| `Config`         | 配置文件                                                                      | -                   |
| `EconomySystem`  | 经济系统 (已封装对接双经济)                                                   | -                   |
| `EventListener`  | 事件监听器(监听拦截 Mc 事件)                                                  | -                   |
| `LandPos`        | 坐标基类 (负责处理 JSON 反射)                                                 | -                   |
| `LandAABB`       | 领地坐标 (一个领地的对角坐标)                                                 | -                   |
| `LandSelector`   | 领地选区器(负责圈地、修改范围)                                                | -                   |
| `DrawHandle`     | 绘制管道 (管理领地绘制, 每个玩家都独立分配一个 DrawHandle)                    | -                   |
| `SafeTeleport`   | 安全传送系统                                                                  | -                   |

!> ⚠️：`Land` 类提供了两个 `using`, 分别为 `SharedLand` 和 `WeakLand`。  
当您需要长期持有 `Land` 时建议使用 `WeakLand` 弱共享智能指针。

```cpp
using SharedLand = std::shared_ptr<class Land>
using WeakLand = std::weak_ptr<class Land>
```

## RAII 资源

插件中许多资源采用了 RAII 机制，它们由 `PLand` 主入口类管理。
当您有需要时，可以从 `PLand` 获取它们
