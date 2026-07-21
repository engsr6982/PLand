# PLand

**[简体中文](README.md)** | **[English](README_EN.md)**

[![CI](https://img.shields.io/github/actions/workflow/status/IceBlcokMC/pland/build.yml?branch=main&style=for-the-badge&logo=github&label=CI)](https://github.com/IceBlcokMC/pland/actions)
[![License](https://img.shields.io/badge/license-AGPLv3-blue?style=for-the-badge&logo=open-source-initiative)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/20)
[![Release](https://img.shields.io/github/v/release/IceBlcokMC/pland?include_prereleases&style=for-the-badge)](https://github.com/IceBlcokMC/pland/releases)

---

PLand 是基于 **LeviLamina** 生态开发的高性能领地插件，为 BDS 提供细粒度的领地保护、多角色权限控制、灵活的经济与租赁体系、数据迁移支持以及完善的第三方扩展接口。

---

## ✨ 特性一览

### 📐 多种领地形态

- 2D 领地: 包含**整个维度高度**的矩形区域。
- 3D 领地: 包含**指定高度范围**的矩形区域。
- 子领地: 可以在父领地内创建子领地，支持多级嵌套。

### 多种便利功能

- **重新选区**: 自由调整当前领地范围，多退少补。
- **领地传送点**：设置领地内传送点，快速回到领地。
- **领地转让**：领地主人可以转让领地给其他玩家。
- **领地边界显示**：可视化领地边界，方便玩家查看领地范围。
- **权限管理**：精细控制玩家在领地内的行为权限。
- **领地别名**：个性化领地名称，快速区分不同领地。

> 更多功能请查看游戏内领地管理菜单。

### ⚙️ 个性化领地运营

- **模式切换**：支持**永久买断**模式与**按期租赁**模式。
- **规则与约束配置**：支持设置维度禁建区/白名单、仅租赁区域、领地间距、最小/最大空间范围、领地创建数量上限以及子领地层级限制。

> 更多配置项请查看配置文件和文档 [点我前往](https://iceblcokmc.github.io/PLand/)

### 👥 4 角色权限模型

采用清晰的四级角色模型，权限可向子领地继承或单独覆盖：

1. **管理员**：拥有全局最高权限与专属管理界面。
2. **领地主人**：拥有领地最高控制权、分配成员权限与转让领地权。
3. **领地成员**：可由主人独立配置细分操作权限。
4. **实体**：默认公共权限组，控制任意实体、访客玩家的权限。

### 🛡️ 精细化保护机制 (60+ 权限节点)

PLand 提供针对**玩家行为**与**环境行为**的细粒度控制：

| 玩家行为控制            | 环境行为控制                   |
| ----------------------- | ------------------------------ |
| 方块放置 / 破坏         | 爆炸破坏 (TNT/苦力帕)          |
| 容器访问 (箱子/木桶等)  | 火焰蔓延 / 苔藓蔓延            |
| 工作站与特殊方块交互    | 生物生成 / 滴水石 / 活塞推动   |
| PvP 战斗与远程武器使用  | 液体流动 (水/岩浆)             |
| 实体交互 / 交通工具乘坐 | 幽匿蔓延 / 龙蛋传送            |
| 投掷物使用 / 方块掉落   | 凋零破坏 / 闪电击中 / 耕地退化 |

> 仅展示部分权限节点，完整权限节点请在游戏内查看。

### 💰 经济与计价系统

- **双经济系统**：灵活对接服务器经济插件 (计分板、LegacyMoney 等)。
- **自定义公式**：每种领地类型与维度均可配置独立的价格计算公式与倍率。
- **折扣与退款**：提供完整的购买折扣与领地回收/退租退款率配置。

### 💻 交互与体验

- **GUI 表单**：内置全面封装的 GUI 菜单，提供方便快捷的玩家交互体验、高级过滤器与管理员专属表单。
- **命令支持**：提供丰富的领地管理命令，支持领地创建、购买、绘制等操作。
- **多语言支持 (i18n)**：内置简体中文（Native），支持美式英语与俄语等语言包。

> 更多功能，请在游戏内查看领地管理菜单与命令。

---

## 🔧 动态权限映射

PLand 内部采用**动态权限映射机制**。方块、物品与实体类型均可直接通过配置文件映射至指定权限节点。

```json
{
  "minecraft:barrel": "useContainer",
  "minecraft:lectern": "useLectern",
  "minecraft:bee_nest": "useBeeNest"
}
```

> 具体请查看配置文件与文档 [点我前往](https://iceblcokmc.github.io/PLand/)

---

## 🛡️ 多层拦截架构

为了最大限度覆盖游戏行为并修复常规事件越权漏洞，PLand 构建了多层拦截防御体系：

- **事件监听层 (Event Layer)**：基于加载器生态，覆盖 40+ 底层常规事件，包括方块操作、玩家交互、实体行为、生物生成及战斗伤害等。
- **底层拦截层 (Hook Layer)**：对于事件系统无法完整覆盖的机制（如漏斗跨界抽取、闪电击中、耕地踩踏退化、特殊方块/实体交互等），通过 C++ 底层 Hook 进行补充拦截。
- **可配置启用**：所有监听器与 Hook 均支持单独开启或关闭，方便性能调优与兼容性排查。

---

## ⚡ 性能与数据存储

- **存储方案**：采用 **内存缓存 + LevelDB 持久化** 双层架构。主线程读写均在内存中高频完成，由后台线程异步写入数据库。
- **算法优化**：使用领地查询哈希索引、子领地层级预计算与双向表结构，加速复杂领地的搜索与删除。
- **数据版本管理**：所有领地数据带有版本印记。升级插件时，系统会自动将历史数据平滑升级迁移至新版结构，保证向后兼容。

---

## 🔌 开发者生态与 API

PLand 提供了公开的 Service 接口与事件注册机制，方便第三方插件调用：

- **核心 API**：查询领地信息、执行权限校验、获取领地归属关系及运行数据。
- **DevTool 工具包 (`src-devtool`)**：提供领地可视化查看、范围实时编辑与领地树状层级可视化分析。

---

## 📂 工程结构

```bash
PLand
├─ assets/             # 插件资源与多语言 JSON 语言包
├─ docs/               # 用户文档与开发者文档
├─ src/
│  └─ pland/
│     ├─ aabb/          # 空间与碰撞计算
│     ├─ drawer/        # 领地边界绘制渲染
│     ├─ economy/       # 双经济插件适配
│     ├─ events/        # 领域事件、经济与玩家事件
│     ├─ gui/           # 玩家与管理员 GUI 菜单
│     ├─ infra/         # 基础设施与数据迁移器 (Migrator)
│     ├─ internal/      # 命令处理与多层拦截器 (Interceptor)
│     ├─ land/          # 领地核心逻辑、仓库与验证器
│     ├─ selector/      # 选区工具
│     └─ service/       # 开放底层服务与 API 接口
└─ src-devtool/        # 开发者可视化分析与调试工具

```

---

## 📖 文档与帮助

如果您遇到问题、发现 Bug 或有新的想法，欢迎参与项目建设：

- 📘 详细文档：[PLand Docs 站点](https://iceblcokmc.github.io/PLand/)
- 🐞 提交问题：[GitHub Issues](https://github.com/IceBlcokMC/PLand/issues)
- 💬 社区讨论：[GitHub Discussions](https://github.com/IceBlcokMC/PLand/discussions)
- 💬 QQ 群：[424958821](https://qm.qq.com/q/3KTlMbGt0A)

---

## 📄 开源许可证

本项目采用 [AGPL-3.0 or later](LICENSE) 协议开源。

> **免责声明**：开发者不对使用本软件造成的任何损失或后果负责。使用本项目及其衍生版本即代表您同意自行承担相关风险。

[![Star History Chart](https://api.star-history.com/svg?repos=IceBlcokMC/PLand&type=Date)](https://star-history.com/#IceBlcokMC/PLand&Date)
