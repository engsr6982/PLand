# 🛠 贡献指南 | Contribution Guide

感谢你对本项目的关注！为了保持代码质量和开发效率，请在贡献前阅读以下指南。

Thank you for your interest in this project! To keep our codebase maintainable and collaborative, please follow the guidelines below before making contributions.

---

## 📌 提交流程 | Contribution Workflow

1. **提 Issue**

   - 所有 bug、功能建议必须通过 GitHub Issue 提交。
   - 群聊反馈的问题需由维护者/提出者手动创建 Issue 进行跟踪。

2. **分支策略**

   - 请从 `main` 分支拉取新的分支进行开发，命名建议：
     - `fix/xxx`：修复相关
     - `feat/xxx`：新增功能
     - `refactor/xxx`：重构优化
   - 对于大量更改(重构组件)，请不要直接在 `main` 分支上提交代码。

3. **PR 要求**
   - 每个 PR 只处理一个功能或问题，保持粒度清晰。
   - PR 标题使用中文或英文说明，格式参考：
     - `fix: 修复无法攻击 addon 生物的问题`
     - `feat: 增加土地边界判断逻辑`
   - 请确保 CI 通过后再请求合并。
   - 如果修改了涉及 i18n 的内容，请同步更新 `assets/lang` 目录下的语言文件。
   - 如果修改了依赖版本，请务必锁定依赖版本号或者提交哈希，例如: `add_requires("levilamina 1.0.0")`
   - PR 的历史记录应保持清晰，避免不必要的合并提交。
   - 对于某些脏历史记录，请使用 `git rebase` 或 `git cherry-pick` 等命令进行清理。

---

## 📝 代码规范 | Code Conventions

- 请遵循 [LeviLamina C++ Style Guide](https://lamina.levimc.org/zh/maintainer_guides/cpp_style_guide/)。
- 请使用 [clang-format](https://clang.llvm.org/extra/clang-format/) 格式化代码。
- 例外的情况：对于特定结构体，例如 `Vec3`，内部字段可以直接使用 `x`, `y`, `z`，无需使用 `mX`, `mY`, `mZ`。
- 对于头文件，务必使用 `#pragma once` 以防止重复包含。
- 尽可能使用前向声明(`forward declaration`)，避免头文件相互包含，导致编译时间增加。

---

## 🧾 提交信息规范 | Commit Message Conventions

使用以下格式规范提交说明：

```
<type>: <简洁描述>

例如：
fix: 修复龙蛋交互无权限问题
feat: 新增末影人搬运权限控制
```

支持的类型（type）包括：

- `feat`: 新功能
- `fix`: 问题修复
- `refactor`: 重构优化（无新增功能或修复）
- `chore`: 杂项，如格式化、依赖更新
- `docs`: 文档修改
- `test`: 添加或修改测试

---

## 📄 Changelog 编写 | Changelog Updates

每次提交 PR 时，请确保更新 `CHANGELOG.md` 文件，以便记录项目更新日志。

> 对于 ChangeLog 的格式，参考 https://keepachangelog.com/en/1.0.0/

> 注意：如果更改的内容有对应的 Issue，请将 Issue 编号添加到提交信息中，例如 `fix: xxxxx #123 @xxxx`。

---

再次感谢你的贡献！

Thanks again for contributing!
