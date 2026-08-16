# 环境配置

::: warning 本文为 C++ 开发环境配置  
如果您需要在 LegacyScriptEngine-QuickJs 上调用 PLand 接口  
可以使用 [PLand-LegacyRemoteCallApi](https://github.com/engsr6982/PLand-LegacyRemoteCallApi)
:::

::: warning 在开始之前，请确保您的 PC 上已经安装了以下软件：

- Git
- Visual Studio 2022 (带 C++ 桌面开发工作负载)
- XMake
- VS Code (可选)

:::

::: warning 本教程默认您会 C++ 编程，并且熟悉 Git 和 XMake。
:::

## 创建项目

::: tip 此部分查看 LeviLamina 文档: [创建项目](https://lamina.levimc.org/zh/developer_guides/tutorials/create_your_first_mod/)
:::

## 引入 SDK

1. 打开刚刚创建的项目文件夹，找到 `xmake.lua` 文件并打开。

2. 在文件顶部添加以下代码：

```lua
add_repositories("engsr6982-repo https://github.com/engsr6982/xmake-repo.git")

add_requires("pland 0.0.3")

target("xxx")
    -- ...
    add_packages("pland")
```

::: tip `xxx` 是您在 `target` 中定义的生成目标，请将其替换为您自己的包名。  
:::

3. 保存并关闭文件。

4. 打开终端，进入项目文件夹，运行以下命令：

```bash
xmake repo -u
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
