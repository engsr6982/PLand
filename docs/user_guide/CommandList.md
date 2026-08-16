# 命令列表

PLand 提供以下命令进行交互

```log
? pland
20:14:39.468 INFO [Server] pland (also land):
20:14:39.468 INFO [Server] PLand - 领地系统
20:14:39.468 INFO [Server] Usage:
20:14:39.468 INFO [Server] - /pland
20:14:39.468 INFO [Server] - /pland admin <add|remove> <targets: target>
20:14:39.468 INFO [Server] - /pland admin lease <force_freeze|force_recycle> [id: int]
20:14:39.468 INFO [Server] - /pland admin lease <set_start|set_end> <date: string> [id: int]
20:14:39.468 INFO [Server] - /pland admin lease add_time <day|hour|min|sec> <amount: int> [id: int]
20:14:39.468 INFO [Server] - /pland admin lease clean <days: int>
20:14:39.468 INFO [Server] - /pland admin lease to_bought [id: int]
20:14:39.468 INFO [Server] - /pland admin lease to_leased <days: int> [id: int]
20:14:39.468 INFO [Server] - /pland admin list
20:14:39.468 INFO [Server] - /pland buy
20:14:39.468 INFO [Server] - /pland cancel
20:14:39.468 INFO [Server] - /pland debug dump_selectors
20:14:39.468 INFO [Server] - /pland devtool [on|off]
20:14:39.468 INFO [Server] - /pland draw <disable|near_land|current_land>
20:14:39.468 INFO [Server] - /pland gui
20:14:39.468 INFO [Server] - /pland lease info [id: int]
20:14:39.468 INFO [Server] - /pland menu
20:14:39.468 INFO [Server] - /pland mgr
20:14:39.468 INFO [Server] - /pland new [default|sub_land]
20:14:39.468 INFO [Server] - /pland reload
20:14:39.468 INFO [Server] - /pland set <a|b>
20:14:39.468 INFO [Server] - /pland set teleport_pos
20:14:39.468 INFO [Server] - /pland this
```

## 命令详解

> `pland` 为顶层命令，`land` 是它的别名
>
> 使用尖括号 `<...>` 包裹的参数为必须输入  
> 使用方括号 `[...]` 包裹的参数为可选输入  
> 使用竖线 `|` 分割的参数为枚举，即不同的重载

### `/pland [gui|menu]`

**打开领地系统主菜单**

| 项目   | 说明 |
|------|----|
| 执行主体 | 玩家 |
| 所需权限 | /  |
| 额外依赖 | /  |

### `/pland mgr`

**打开领地管理 GUI**

| 项目   | 说明  |
|------|-----|
| 执行主体 | 玩家  |
| 所需权限 | 管理员 |
| 额外依赖 | /   |

### `/pland new [default|sub_land]`

**新建领地**

- `default` 默认普通领地
- `sub_land` 子领地

| 项目   | 说明                                   |
|------|--------------------------------------|
| 执行主体 | 玩家                                   |
| 所需权限 | `default` 任意玩家, `sub_land` 仅领地主人或管理员 |
| 额外依赖 | /                                    |

### `/pland buy`

**购买框选的领地范围**

| 项目   | 说明                  |
|------|---------------------|
| 执行主体 | 玩家                  |
| 所需权限 | /                   |
| 额外依赖 | 先执行 `/pland new` 命令 |

### `/pland cancel`

**取消当前选区任务**

| 项目   | 说明                  |
|------|---------------------|
| 执行主体 | 玩家                  |
| 所需权限 | /                   |
| 额外依赖 | 先执行 `/pland new` 命令 |

### `/pland set <a|b>`

**设定选区 A/B 点**

- `a` 设定选区 A 点
- `b` 设定选区 B 点

| 项目   | 说明                  |
|------|---------------------|
| 执行主体 | 玩家                  |
| 所需权限 | /                   |
| 额外依赖 | 先执行 `/pland new` 命令 |

### `/pland reload`

**热重载领地配置文件**

> 此重载非彻底重新加载数据，重载后可能存在状态不一致  
> 推荐使用: `ll reactivate PLand` 命令进行彻底重载

| 项目   | 说明  |
|------|-----|
| 执行主体 | 控制台 |
| 所需权限 | /   |
| 额外依赖 | /   |

### `/pland this`

**打开当前位置的领地管理GUI**

| 项目   | 说明         |
|------|------------|
| 执行主体 | 玩家         |
| 所需权限 | 领地主人 / 管理员 |
| 额外依赖 | /          |

### `/pland draw <disable|near_land|current_land>`

**绘制附近的领地范围**

- `disable` 关闭绘制
- `current_land` 绘制当前所在的领地范围
- `near_land` 绘制指定范围内的领地(`features.draw.range`)

| 项目   | 说明                          |
|------|-----------------------------|
| 执行主体 | 玩家                          |
| 所需权限 | /                           |
| 额外依赖 | 配置文件`features.draw.enabled` |

### `/pland set teleport_pos`

**修改当前领地的传送点为当前所在位置**

| 项目   | 说明       |
|------|----------|
| 执行主体 | 玩家       |
| 所需权限 | 领地主人、管理员 |
| 额外依赖 | /        |

### `/pland admin list`

**列出所有领地管理员**

| 项目   | 说明  |
|------|-----|
| 执行主体 | 控制台 |
| 所需权限 | /   |
| 额外依赖 | /   |

### `/pland admin <add|remove> <targets: target>`

**添加或移除一个领地管理员**

- `add` 添加
- `remove` 移除
- `targets` 玩家选择器

| 项目   | 说明  |
|------|-----|
| 执行主体 | 控制台 |
| 所需权限 | /   |
| 额外依赖 | /   |

### `/pland devtool [on|off]`

**打开开发者工具**

- `on` 开启
- `off` 关闭

| 项目   | 说明                     |
|------|------------------------|
| 执行主体 | 控制台                    |
| 所需权限 | /                      |
| 额外依赖 | 配置文件 `system.devTools` |

### `/pland lease info [id: int]`

**查看当前领地的租赁信息**

- `id` 指定要操作的领地 ID (留空则操作当前所在的领地)

| 项目   | 说明                              |
|------|---------------------------------|
| 执行主体 | 控制台 / 玩家                        |
| 所需权限 | 仅查询他人领地时需要管理员权限                 |
| 额外依赖 | 配置文件 `business.leasing.enabled` |

### `/pland admin lease <set_start|set_end> <date: string> [id: int]`

**修改领地租赁的开始或结束时间**

- `set_start` 设置开始时间
- `set_end` 设置结束时间
- `date` 日期或时间戳字符串
    - 对于日期格式`yyyy-MM-dd HH:mm:ss`
    - 对于时间戳**单位为秒** (不支持毫秒时间戳)
- `id` 指定要操作的领地 ID (留空则操作当前所在的领地)

| 项目   | 说明                              |
|------|---------------------------------|
| 执行主体 | 控制台 / 玩家                        |
| 所需权限 | 管理员                             |
| 额外依赖 | 配置文件 `business.leasing.enabled` |

### `/pland admin lease <force_freeze|force_recycle> [id: int]`

**强制冻结或回收一个领地**

- `force_freeze` 强制冻结
- `force_recycle` 强制回收
- `id` 指定要操作的领地 ID (留空则操作当前所在的领地)

| 项目   | 说明                              |
|------|---------------------------------|
| 执行主体 | 控制台 / 玩家                        |
| 所需权限 | 管理员                             |
| 额外依赖 | 配置文件 `business.leasing.enabled` |

### `/pland admin lease add_time <amount: int> <day|hour|min|sec> [id: int]`

**添加领地租赁时间(租期)**

- `amount` 要添加的数量
- `day|hour|min|sec` 添加的时间单位
- `id` 指定要操作的领地 ID (留空则操作当前所在的领地)

| 项目   | 说明                              |
|------|---------------------------------|
| 执行主体 | 控制台 / 玩家                        |
| 所需权限 | 管理员                             |
| 额外依赖 | 配置文件 `business.leasing.enabled` |

### `/pland admin lease clean <days: int>`

**清理到期超过n天的领地(不含冻结期)**

- `days` 天数

| 项目   | 说明                              |
|------|---------------------------------|
| 执行主体 | 控制台 / 玩家                        |
| 所需权限 | 管理员                             |
| 额外依赖 | 配置文件 `business.leasing.enabled` |

### `/pland admin lease to_bought [id: int]`

**将租赁领地转为买断制领地**

- `id` 指定要操作的领地 ID (留空则操作当前所在的领地)

| 项目   | 说明                              |
|------|---------------------------------|
| 执行主体 | 控制台 / 玩家                        |
| 所需权限 | 管理员                             |
| 额外依赖 | 配置文件 `business.leasing.enabled` |

### `/pland admin lease to_leased <days: int> [id: int]`

**将买断制领地转为租赁领地**

- `days` 租赁天数
- `id` 指定要操作的领地 ID (留空则操作当前所在的领地)

| 项目   | 说明                              |
|------|---------------------------------|
| 执行主体 | 控制台 / 玩家                        |
| 所需权限 | 管理员                             |
| 额外依赖 | 配置文件 `business.leasing.enabled` |

### `/pland admin snapshot_db [dirName: string]`

**对数据库创建快照(异步)**

- `dirName` 快照保存目录名 (如果留空则使用时间戳)

> 所有快照均保存在 `plugins/PLand/data/snapshots/<dirName>` 目录下
>
> 如果任务未完成，目录下会存在 `.incomplete` 文件，表示任务正在进行中

| 项目   | 说明       |
|------|----------|
| 执行主体 | 控制台 / 玩家 |
| 所需权限 | 管理员      |
| 额外依赖 | /        |

## 已移除命令

### `/pland import <clearDb: Boolean> <relationship_file: string> <data_file: string>`

**导入 iLand 领地数据**

> 此命令已在 v0.18.0 版本移除，如有转换需求，请使用 v0.17.0 的PLand执行转换后更新到最小版本

- `clearDb` 是否清空数据库
- `relationship_file` 领地关系文件(路径)
- `data_file` 领地数据文件(路径)

> 例如：/pland import true "C:/Users/xxx/Desktop/relationship.json" "C:/Users/xxx/Desktop/data.json"

| 项目   | 说明  |
|------|-----|
| 执行主体 | 控制台 |
| 所需权限 | /   |

::: warning 注意

此转换为动态转换，由于iLand使用XUID作为玩家唯一标识符，而本插件使用UUID。  
因此，插件转换会后Owner数据依然为XUID，待玩家进服后，插件会自动转换为UUID。  
所以：请不要关闭xbox验证，否则无法转换成功。  
为了避免意外情况，我们仅建议在服务器刚开服时导入数据。  
或者导入时，将 `clearDb` 设置为 `true`，清空数据库，重新导入。  
否则可能会出现已有领地和导入的领地范围重叠等问题。

:::