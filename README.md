# PLand

基于 LeviLamina 开发、适用于 BDS 的领地系统 PLand。

## 功能特性

|    功能    |           描述           | 状态 |
| :--------: | :----------------------: | :--: |
|  2D 领地   |       创建 2D 领地       |  ✅  |
|  3D 领地   |       创建 3D 领地       |  ✅  |
|   子领地   |     在领地内创建领地     |  ✅  |
| 嵌套子领地 |     嵌套创建多层领地     |  ✅  |
|  领地保护  |    领地保护，防止破坏    |  ✅  |
|  领地转让  |  领地转让，领地主人转让  |  ✅  |
|  领地传送  |    安全的领地传送功能    |  ✅  |
|    GUI     |     精美表单，易操作     |  ✅  |
|  事件覆盖  |       事件覆盖率高       |  ✅  |
|  经济系统  |      支持双经济系统      |  ✅  |
|  权限管理  |      精细的权限管理      |  ✅  |
|   多语言   | 简中、英语、俄语、文言文 |  ✅  |
|    SDK     | 完整 SDK 接口支持，易用  |  ✅  |
|    性能    |      C++ 编写，高效      |  ✅  |
|  价格公式  |    自定义公式计算价格    |  ✅  |
|  禁止区域  | 禁止在划定区域内创建领地 |  ✅  |
|  DevTools  |   开发者工具，方便调试   |  ✅  |

> 更多内容，请移步文档站：https://engsr6982.github.io/PLand/

## 工程结构

```bash
C:\PLand
├─assets                      # 插件资源文件（语言、文本）
│  └─lang                     # 多语言支持的 JSON 语言包
│
├─devtool                     # 开发者工具模块
│  ├─components               # 封装的 ImGUI 可复用组件
│  ├─deps                     # 工具内部使用的第三方库
│  └─impl                     # 工具功能实现
│      ├─helper               # 辅助功能页面
│      └─viewer               # 领地可视化查看界面
│          └─internals        # Viewer 的内部状态/数据模型
│
├─docs                        # 项目文档
│  ├─dev                      # 开发者文档
│  └─md                       # 用户文档
│
├─scripts                     # 构建辅助脚本
│
├─src
│  └─pland
│      ├─aabb                 # 领地空间计算（AABB、坐标等）
│      ├─command              # Minecraft 命令注册与处理逻辑
│      ├─economy              # 经济系统整合（价格逻辑、支付对接）
│      ├─gui                  # 表单式玩家交互界面
│      │  └─form              # 通用表单组件封装（分页、返回等）
│      ├─hooks                # 权限钩子/事件拦截
│      ├─infra                # 基础设施（配置、工具类、绘制支持等）
│      ├─land                 # 核心业务：Land 实体、事件、注册表等
│      ├─mod                  # 插件入口
│      ├─selector             # 区域选择器
│      └─utils                # 通用工具模块
│
└─test                        # 测试代码
```

## 开源协议

本项目采用 [AGPL-3.0](LICENSE) 开源协议。

> 开发者不对使用本软件造成的任何后果负责，当您使用本项目以及衍生版本时，您需要自行承担风险。

> 感谢以下开源项目对本项目的支持与帮助。

|           项目名称           |                          项目地址                           |
| :--------------------------: | :---------------------------------------------------------: |
|          LeviLamina          |           https://github.com/LiteLDev/LeviLamina            |
|            exprtk            |            https://github.com/ArashPartow/exprtk            |
|         LegacyMoney          |           https://github.com/LiteLDev/LegacyMoney           |
|   iListenAttentively(闭源)   | https://github.com/MiracleForest/iListenAttentively-Release |
| BedrockServerClientInterface |   https://github.com/OEOTYAN/BedrockServerClientInterface   |
|            ImGui             |              https://github.com/ocornut/imgui               |
|             glew             |             https://github.com/nigels-com/glew              |
|      ImGuiColorTextEdit      |       https://github.com/goossens/ImGuiColorTextEdit        |

## 贡献

欢迎提交 Issue 和 Pull Request，共同完善 PLand。
