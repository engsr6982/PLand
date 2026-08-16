# LDAPI

::: tip `LDAPI` 为 PLand 导出的 API  
`LDAPI` 的本质是一个宏，值为 `__declspec(dllimport)`  
头文件中，标记 `LDAPI` 的函数您都能访问、调用
:::

## class 指南

::: tip 由于 PLand 尚未进入 v1.0.0 内部API变化频繁，本文档无法实时更新，请以 SDK 为准
:::

| 类名               | 描述                                                           | 备注               |
|:-----------------|:-------------------------------------------------------------|:-----------------|
| `LandRegistry`   | LandRegistry 核心类(负责存储、查询)                                    | -                |
| `Land`           | 领地代理类(提供对原始数据的封装 API)                                        | |
| `PriceCalculate` | 价格公式解析、计算 ([formula 价格表达式](../user_guide/Config.md#formula-价格表达式)) | -                |
| `Config`         | 配置文件                                                         | -                |
| `EconomySystem`  | 经济系统 (已封装对接双经济)                                              | -                |
| `EventListener`  | 事件监听器(监听拦截 Mc 事件)                                            | -                |
| `LandPos`        | 坐标基类 (负责 JSON 序列化)                                          | -                |
| `LandAABB`       | 领地坐标 (一个领地的对角坐标)                                             | -                |
| `LandSelector`   | 领地选区器(负责圈地、修改范围)                                             | -                |
| `DrawHandle`     | 绘制管道 (管理领地绘制, 每个玩家都独立分配一个 DrawHandle)                        | -                |

::: warning ⚠️：当您需要长期持有 `Land` 时建议使用 `std::weak_ptr<Land>` 弱引用。
:::

## RAII 资源

插件中许多资源采用了 RAII 机制，它们由 `PLand` 主入口类管理。  
当您有需要时，可以从 `PLand` 获取它们

## `internal` 相关

`internal` 命名空间 / 文件夹 下的文件、方法、API 为内部 API  
PLand 默认不导出这些API的符号，也不推荐访问、修改 `internal` 下的内容

## `service` 相关

> v0.18+ 版本开始，PLand 重构了许多代码，引入了 service 模式

您可以通过 `PLand::getInstance().getServiceLocator()` 获取 `ServiceLocator`

所有 `Service` 都受 `ServiceLocator` 管理，您可以通过 `ServiceLocator` 获取它们