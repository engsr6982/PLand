---
prev:
  text: i18n
  link: /developer_guide/I18n
next:
  text: PLand-NAPI
  link: /developer_guide/script/NAPI
---

# 脚本开发

> 不想写 C++？你可以在 **LegacyScriptEngine（LSE）** 中使用 **JavaScript / TypeScript** 脚本调用 PLand 的 API，开发自己的领地附属功能。

::: tip 前置条件
脚本开发需要服务器已经安装：

- **PLand**（本体）
- **LegacyScriptEngine**（`lse-quickjs` 或 `lse-nodejs` 引擎）
  :::

## 两种方案

PLand 目前提供两个脚本调用方案：

| 方案                                                                         | 状态                                   | 说明                                                                                                                      |
| :--------------------------------------------------------------------------- | :------------------------------------- | :------------------------------------------------------------------------------------------------------------------------ |
| [**PLand-NAPI**](/developer_guide/script/NAPI)                               | <Badge type="warning" text="实验性" /> | PLand 的**原生绑定 API** 方案，脚本可直接持有领地对象、性能更好。但**目前仍处于早期实验阶段**，还有一些问题没有验证和解决 |
| [**PLand-LegacyRemoteCallApi**](/developer_guide/script/LegacyRemoteCallApi) | <Badge type="info" text="正常维护" />  | 基于 `LegacyRemoteCall` 远程调用桥接的方案                                                                                |

## 如何选择？

::: warning 请注意
PLand-NAPI 目前**只是一个理想的推荐方案**，还有问题未验证解决。当前阶段请以 **PLand-LegacyRemoteCallApi** 作为稳定可用的脚本开发方案。
:::

| 你的情况                | 建议                                                            |
| :---------------------- | :-------------------------------------------------------------- |
| 开发稳定可用的脚本功能  | ✅ **PLand-LegacyRemoteCallApi**（成熟稳定，持续维护）          |
| 想尝鲜体验原生绑定      | 可以尝试 **PLand-NAPI**，但注意其处于实验期，可能存在未解决问题 |
| 大型项目 / 对性能要求高 | 建议直接使用 [C++ 原生开发](/developer_guide/QuickStart)        |

::: tip 性能提示

- **PLand-NAPI** 为原生绑定，脚本可**直接持有**领地对象，性能接近 C++ 调用（待验证成熟）
- **LegacyRemoteCallApi** 因引擎限制采用折衷方案（句柄），大型项目存在性能问题
  :::

## 下一步

- 了解实验性的原生绑定方案 → [PLand-NAPI](/developer_guide/script/NAPI)
- 使用稳定可用的桥接方案 → [PLand-LegacyRemoteCallApi](/developer_guide/script/LegacyRemoteCallApi)
- 想要最强性能 → [C++ 快速开始](/developer_guide/QuickStart)
