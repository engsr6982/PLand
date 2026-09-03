# 其它接口

## 获取 PLand 版本元信息 <Badge type="warning" text="deprecated v0.22.1" />

`PLand_getVersionMeta()`

- 参数: 无
- 返回值: 版本元信息 JSON 字符串：`{"Commit": "...", "Branch": "...", "Tag": "..."}`
- 返回值类型: `String`

## 获取 PLand 构建信息 <Badge type="tip" text="v0.22.1+" />

`PLand_getBuildInfo()`

- 参数: 无
- 返回值: 构建信息 JSON 字符串
  ```json
  {
    "kBuildMode": "release", // 构建模式，release 或 debug
    "kBuildCommit": "266cb01", // 构建时的 git commit hash
    "kBuildBranch": "HEAD", // 构建时的 git branch
    "kBuildTag": "v0.21.1-50-g266cb01" // 构建时的 git tag
  }
  ```
- 返回值类型: `String`
