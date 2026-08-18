# 安装指南

PLand 有两种安装方式，请根据你的情况选择：

| 方式 | 适合人群 | 难度 | 说明 |
|:----|:-------|:----:|:----|
| [Lip 一键安装](#使用-lip-安装推荐) | **绝大多数服主（推荐）** | ⭐ | 自动下载并处理全部依赖，一条命令搞定 |
| [手动安装](#手动安装) | 熟悉插件结构的进阶用户 | ⭐⭐⭐ | 手动下载并放置插件与前置组件 |

::: tip 想先体验一下？
安装完成后，可以跟着 [快速上手](/user_guide/QuickStart) 教程，5 分钟学会圈地和管理领地。
:::

## 使用 Lip 安装（推荐）

> 使用前请确保已经安装了 [Lip](https://lip.levimc.org/)。

### 安装最新版本

```bash
lip install github.com/IceBlcokMC/PLand
```

### 安装指定版本

```bash
lip install github.com/IceBlcokMC/PLand@v1.0.0
```

### 更新 PLand

```bash
lip install --upgrade github.com/IceBlcokMC/PLand
```

安装完成后：

1. 在 `plugins` 文件夹中可以看到 PLand 的安装文件
2. 双击 `bedrock_server_mod.exe` 启动服务器
3. 在服务器日志中看到 [PLand 的启动日志](#启动日志) 即安装成功

::: tip Lip 会自动处理 PLand 的依赖（LeviLamina、iListenAttentively 等），无需手动下载。
:::

## 手动安装

::: warning 手动安装需要你先自行装好 PLand 的全部前置组件，且版本必须与 PLand 兼容。
:::

### 前置组件

| 前置组件                     | 项目地址                                                              | 依赖等级 | 备注                                        |
| :--------------------------- | :-------------------------------------------------------------------- | :------: |:------------------------------------------|
| LeviLamina                   | [GitHub](https://github.com/LiteLDev/LeviLamina)                      |   必须   | Mod 框架                                    |
| iListenAttentively           | [GitHub](https://github.com/MiracleForest/iListenAttentively-Release) |   必须   | 事件库                                       |
| LegacyMoney                  | [GitHub](https://github.com/LiteLDev/LegacyMoney)                     |   可选   | `economy.kit` 为 `LegacyMoney` 时需要         |
| DebugShape                   | [Github](https://github.com/IceBlcokMC/DebugShape)                     |   可选   | `features.draw.backend == DebugShape` 时需要 |

> 在下载前置组件时，请确保版本与 PLand 兼容。  
> 如果你不确定，**强烈建议改用 Lip 安装**，由 Lip 自动处理依赖。

### 安装步骤

1. 前往 [Minebbs](https://www.minebbs.com/) 或 [Github Release](https://github.com/IceBlcokMC/PLand/releases) 下载 PLand 的最新版本

2. 解压下载的 `PLand-windows-x64.zip`

3. 将解压后的 `PLand` 文件夹移动到 `plugins` 文件夹下

4. 双击 `bedrock_server_mod.exe` 启动服务器

5. 在服务器日志中看到 [PLand 的启动日志](#启动日志) 即安装成功

## 启动日志

出现以下日志说明 PLand 已成功加载：

```log
22:03:02.933 INFO [PLand]   _____   _                        _
22:03:02.933 INFO [PLand]  |  __ \ | |                      | |
22:03:02.933 INFO [PLand]  | |__) || |      __ _  _ __    __| |
22:03:02.933 INFO [PLand]  |  ___/ | |     / _` || '_ \  / _` |
22:03:02.933 INFO [PLand]  | |     | |____| (_| || | | || (_| |
22:03:02.933 INFO [PLand]  |_|     |______|\__,_||_| |_| \__,_|
22:03:02.933 INFO [PLand]
22:03:02.933 INFO [PLand] Loading...
22:03:02.933 INFO [PLand] Build Time: 2024-09-22 22:43:13
22:03:02.964 INFO [PLand] 已加载 1 位操作员
22:03:02.964 INFO [PLand] 已加载 1 块领地数据
22:03:02.964 INFO [PLand] 初始化领地缓存系统完成
22:03:02.964 INFO [LeviLamina] PLand 已加载
```

## 常见安装问题

### Q：安装后没有看到 PLand 的启动日志？

- 检查 `plugins` 文件夹下是否有 `PLand` 文件夹
- 检查是否已安装全部**必须**的前置组件（LeviLamina、iListenAttentively）
- 查看服务器完整日志，确认是否存在加载失败/崩溃的具体报错

### Q：日志提示缺少某个 DLL / 依赖？

PLand 依赖的前置组件版本不匹配，请升级或降级对应前置组件到与 PLand 兼容的版本。
如果不确定版本，建议用 Lip 安装让工具自动处理。

### Q：我想从旧版本升级到新版本？

```bash
lip install --upgrade github.com/IceBlcokMC/PLand
```

::: warning 升级前请务必备份数据
将 `plugins/PLand` 下的 `config` 和 `data` 文件夹复制一份保存，以防升级出现问题。
:::

### Q：更多问题？

请查看 [FAQ](/user_guide/FAQ) 或加入 [QQ 群](https://qm.qq.com/q/v2faa5B2xk) 反馈。
