# C++ 快速开始

> 本篇带你**从零创建第一个能调用 PLand API 的 C++ 附属插件**，完成后你将拥有一个「玩家进出领地时向全服广播」的可用插件。

::: tip 前置知识

- 本教程假设你会 C++，并且熟悉 Git 和 XMake
- 需要已经安装：Git、Visual Studio 2022（含 C++ 桌面开发工作负载）、**LLVM clang-cl**（22.x）、XMake
- **LeviLamina 26.20 及以上版本**需要 **LLVM clang-cl 22.x** 编译器进行构建
  :::

## 1. 环境准备

PLand 是运行在 **LeviLamina** 上的插件，我们的附属插件首先是一个标准的 LeviLamina Mod。

项目创建、引入 PLand SDK 等环境搭建步骤请先完成 [环境配置](/developer_guide/RequireSDK)：

1. 基于 [levilamina-mod-template](https://github.com/LiteLDev/levilamina-mod-template) 创建项目
2. 在 `xmake.lua` 中引入 PLand SDK（`add_requires("pland >= 0.16.0")` + `add_packages("pland")`）
3. 运行 `xmake repo -u`、`xmake f -y -p windows -a x64 -m release`、`xmake` 完成构建配置

::: warning 使用注意

- `include` 时，PLand 的项目文件在 `pland` 文件夹下，例如 `#include "pland/PLand.h"`
- PLand 的命名空间为 `land`（所有类和函数都在 `land` 命名空间下）
- 在添加新的依赖后，别忘了更新 IntelliSense，否则 clangd 可能无法找到实现
  :::

::: tip 已经完成环境配置？
跳过本节，直接进入 [编写你的第一个插件](#2-编写你的第一个插件)。
:::

## 2. 编写你的第一个插件

在 `src/` 下创建一个源文件（例如 `Main.cpp`），写入以下完整代码：

```cpp
#include "pland/PLand.h"
#include "pland/land/repo/LandRegistry.h"

#include "ll/api/mod/Mod.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"

#include "pland/events/player/PlayerMoveEvent.h"

#include "fmt/format.h"

namespace my_land_plugin {

// 保存监听器，防止被 GC
ll::event::ListenerPtr mEnterListener;
ll::event::ListenerPtr mLeaveListener;

void onLoad() {
    // 1. 注册「玩家进入领地」事件
    mEnterListener = ll::event::EventBus::getInstance().emplaceListener<
        land::event::PlayerEnterLandEvent>([](land::event::PlayerEnterLandEvent const& ev) {
        // 通过 landId() 获取领地的 ID，再查询领地信息
        auto land = land::PLand::getInstance().getLandRegistry().getLand(ev.landId());
        if (!land) return;

        // 向全服广播玩家进入领地
        auto& logger = ll::mod::Mod::current()->getLogger();
        logger.info("{} 进入了领地「{}」", ev.self().getName(), land->getName());
    });

    // 2. 注册「玩家离开领地」事件
    mLeaveListener = ll::event::EventBus::getInstance().emplaceListener<
        land::event::PlayerLeaveLandEvent>([](land::event::PlayerLeaveLandEvent const& ev) {
        auto land = land::PLand::getInstance().getLandRegistry().getLand(ev.landId());
        if (!land) return;

        auto& logger = ll::mod::Mod::current()->getLogger();
        logger.info("{} 离开了领地「{}」", ev.self().getName(), land->getName());
    });
}

} // namespace my_land_plugin
```

再把入口函数接上你的 Mod 生命周期（具体写法取决于你的项目模板，通常在 `Main.cpp` 中有类似 `LL_REGISTER_MOD` 的入口），在 `load` 阶段调用 `my_land_plugin::onLoad()` 即可。

## 3. 编译运行

```bash
xmake
```

::: tip 首次构建
如果还未配置过构建，请先执行环境配置中的 `xmake f -y -p windows -a x64 -m release`。
:::

将编译产物放入服务器 `plugins` 文件夹，启动服务器。此时：

- 玩家进入/离开任何领地 → 控制台会打印对应日志
- 你可以在此基础上扩展更多功能

## 4. 下一步

- 了解 PLand 导出的全部 API → [LDAPI](/developer_guide/LDAPI)
- 了解所有可监听的事件 → [Event](/developer_guide/Event)
- 学习服务定位器（ServiceLocator）的用法 → [LDAPI：service 相关](/developer_guide/LDAPI#service-相关)
- 不想写 C++？试试 [脚本开发](/developer_guide/script/index)
