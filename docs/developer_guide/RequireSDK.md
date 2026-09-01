# 环境配置

> 本文是 **C++ 原生开发**的环境配置指南。
>
> 如果你需要用 **LegacyScriptEngine（quickjs / nodejs）脚本**调用 PLand 接口，请看 [脚本开发](/developer_guide/script/index) 板块。

::: warning 在开始之前，请确保您的 PC 上已经安装了以下软件：

- Git
- Visual Studio 2022 (带 C++ 桌面开发工作负载)
- **LLVM（clang-cl 工具链）**
- XMake
- VS Code (可选)

:::

::: warning 编译器要求
- **LeviLamina 26.20 及以上版本**需要 **LLVM clang-cl** 编译器（**22.x**）进行构建
- 需要将其加入 PATH，或使用 `xmake f --toolchain=clang-cl` 指定
:::

::: warning 本教程默认您会 C++ 编程，并且熟悉 Git 和 XMake。
:::

::: tip 想直接上手？
可以先跟着 [C++ 快速开始](/developer_guide/QuickStart) 教程，创建你的第一个能调用 PLand API 的插件。
:::

## 创建项目

LeviLamina **没有提供 `xmake create` 模板**，需要通过官方模板仓库创建：

1. 前往 [levilamina-mod-template](https://github.com/LiteLDev/levilamina-mod-template)，点击 **"Use this template"** 基于模板生成你自己的项目仓库
2. Clone 新仓库到本地
3. 在 `xmake.lua` 中修改 **mod 名称**和期望的 **LeviLamina 版本**
4. 添加你自己的代码

::: tip 更详细的创建教程
查看 LeviLamina 官方文档: [创建项目](https://lamina.levimc.org/zh/developer_guides/tutorials/create_your_first_mod/)
:::

## 引入 SDK

1. 打开刚刚创建的项目文件夹，找到 `xmake.lua` 文件并打开。

2. 在文件顶部添加以下代码：

```lua
add_repositories("iceblcokmc https://github.com/IceBlcokMC/xmake-repo.git")

add_requires("pland >= 0.16.0")

target("xxx")
    -- ...
    add_packages("pland")
```

::: tip
- `xxx` 是您在 `target` 中定义的生成目标，请将其替换为您自己的包名
- PLand SDK 由 [IceBlcokMC/xmake-repo](https://github.com/IceBlcokMC/xmake-repo) 提供（仓库已从 engsr6982 转移）
- 建议使用最新版本的 SDK（当前为 0.16.0），并确保与服务器上安装的 PLand 版本兼容
:::

3. 保存并关闭文件。

4. 打开终端，进入项目文件夹，运行以下命令：

```bash
xmake repo -u
```

```bash
xmake f -y -p windows -a x64 -m release
```

```bash
xmake
```

::: warning 在 `include` 时，PLand 的项目文件在 `pland` 文件夹下 (例: `#include "pland/PLand.h"`)  
:::

::: warning PLand 的命名空间为 `land` (PLand 所有的类和函数都在 `land` 命名空间下)
:::

::: warning 在添加新的依赖后，别忘了更新 IntelliSense，否则 clangd 可能无法找到实现。
:::

## 下一步

- 编写你的第一个插件 → [C++ 快速开始](/developer_guide/QuickStart)
- 查看 PLand 导出的全部 API → [LDAPI](/developer_guide/LDAPI)
- 了解可监听的事件 → [Event](/developer_guide/Event)
