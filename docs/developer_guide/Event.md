# Event & 领地事件

PLand 通过 **LeviLamina 的事件系统**向附属插件暴露领地事件，你可以监听这些事件来实现自己的功能（例如：玩家买地时发消息、领地转让时通知管理员等）。

## 核心概念

::: tip 事件分为 `Before` 和 `After` 两个阶段

- `Before` 阶段：事件触发**前**，可以通过 `ev.cancel()` 取消事件
- `After` 阶段：事件触发**后**，无法取消
  :::

::: tip 命名规则
`事件名` + `Before` / `After` + `Event`  
例如: `PlayerBuyLandBeforeEvent`、`PlayerBuyLandAfterEvent`
:::

::: tip 事件触发顺序
`预检查` -> `Before` -> `处理内容` -> `After`
:::

::: warning 非必要请不要修改事件 `const` 修饰的成员，除非你知道你在做什么。
:::

## 事件一览

### 玩家事件

位于 `pland/events/player/`，均继承 `ll::event::PlayerEvent`（可通过 `ev.self()` 获取玩家）。

| 事件                                      | 头文件                                       | 可取消 |
| :---------------------------------------- | :------------------------------------------- | :----: |
| `PlayerEnterLandEvent`                    | `player/PlayerMoveEvent.h`                   |   ×    |
| `PlayerLeaveLandEvent`                    | `player/PlayerMoveEvent.h`                   |   ×    |
| `PlayerBuyLandBeforeEvent`                | `player/PlayerBuyLandEvent.h`                |   √    |
| `PlayerBuyLandAfterEvent`                 | `player/PlayerBuyLandEvent.h`                |   ×    |
| `PlayerRequestCreateLandEvent`            | `player/PlayerRequestCreateLandEvent.h`      |   ×    |
| `PlayerApplyLandRangeChangeBeforeEvent`   | `player/PlayerApplyLandRangeChangeEvent.h`   |   √    |
| `PlayerApplyLandRangeChangeAfterEvent`    | `player/PlayerApplyLandRangeChangeEvent.h`   |   ×    |
| `PlayerRequestChangeLandRangeBeforeEvent` | `player/PlayerRequestChangeLandRangeEvent.h` |   √    |
| `PlayerRequestChangeLandRangeAfterEvent`  | `player/PlayerRequestChangeLandRangeEvent.h` |   ×    |
| `PlayerChangeLandMemberBeforeEvent`       | `player/PlayerChangeLandMemberEvent.h`       |   √    |
| `PlayerChangeLandMemberAfterEvent`        | `player/PlayerChangeLandMemberEvent.h`       |   ×    |
| `PlayerChangeLandNameBeforeEvent`         | `player/PlayerChangeLandNameEvent.h`         |   √    |
| `PlayerChangeLandNameAfterEvent`          | `player/PlayerChangeLandNameEvent.h`         |   ×    |
| `PlayerDeleteLandBeforeEvent`             | `player/PlayerDeleteLandEvent.h`             |   √    |
| `PlayerDeleteLandAfterEvent`              | `player/PlayerDeleteLandEvent.h`             |   ×    |
| `PlayerLeaseLandEvent`                    | `player/PlayerLeaseLandEvent.h`              |   ×    |
| `PlayerRenewLandEvent`                    | `player/PlayerRenewLandEvent.h`              |   ×    |
| `PlayerTransferLandBeforeEvent`           | `player/PlayerTransferLandEvent.h`           |   √    |
| `PlayerTransferLandAfterEvent`            | `player/PlayerTransferLandEvent.h`           |   ×    |

### 领域事件

位于 `pland/events/domain/`，与具体玩家无关，均带 `land()` 访问器。

| 事件                    | 头文件                           | 可取消 |
| :---------------------- | :------------------------------- | :----: |
| `LandRecycleEvent`      | `domain/LandRecycleEvent.h`      |   ×    |
| `LandResizedEvent`      | `domain/LandResizedEvent.h`      |   ×    |
| `LandStateChangedEvent` | `domain/LandStateChangedEvent.h` |   ×    |
| `MemberChangedEvent`    | `domain/MemberChangedEvent.h`    |   ×    |
| `MembersClearedEvent`   | `domain/MemberChangedEvent.h`    |   ×    |
| `OwnerChangedEvent`     | `domain/OwnerChangedEvent.h`     |   ×    |
| `ConfigReloadEvent`     | `domain/ConfigReloadEvent.h`     |   ×    |

### 经济事件

| 事件                    | 头文件                            | 可取消 |
| :---------------------- | :-------------------------------- | :----: |
| `LandRefundFailedEvent` | `economy/LandRefundFailedEvent.h` |   ×    |

::: tip 提示

- `Before` 事件可以在触发前阻止操作（如阻止购买、阻止删除等）
- 领域事件与 `PlayerXxxAfterEvent` 可通过 `ev.land()` 获取领地对象（`std::shared_ptr<Land>`）
  :::

## 监听事件示例

以下示例监听**玩家进入领地**事件，并在控制台打印玩家与领地信息：

```cpp
#include "pland/events/player/PlayerMoveEvent.h"
#include "pland/PLand.h"
#include "pland/land/repo/LandRegistry.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"

ll::event::ListenerPtr mEnterLandListener;

void setup() {
    mEnterLandListener = ll::event::EventBus::getInstance().emplaceListener<land::event::PlayerEnterLandEvent>(
        [](land::event::PlayerEnterLandEvent const& ev) {
            // ev.self() 获取触发事件的玩家，ev.landId() 获取领地 ID
            auto land = land::PLand::getInstance().getLandRegistry().getLand(ev.landId());
            if (!land) return;

            // 通过 land->getName() 等访问器读取领地信息
            // do something
        }
    );
}
```

## 取消事件示例

`Before` 事件可以通过 `ev.cancel()` 取消，从而**阻止**该操作发生。

例如：禁止玩家在**末地（维度 2）**购买领地：

```cpp
#include "pland/events/player/PlayerBuyLandEvent.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"

ll::event::ListenerPtr mBuyLandListener;

void setup() {
    mBuyLandListener = ll::event::EventBus::getInstance().emplaceListener<land::event::PlayerBuyLandBeforeEvent>(
        [](land::event::PlayerBuyLandBeforeEvent const& ev) {
            // 玩家要购买的领地，其维度信息可以通过选区获得
            // 这里仅演示 cancel 的用法：
            if (/* 不满足你的条件 */) {
                ev.cancel(); // 取消购买
            }
        }
    );
}
```

::: warning 记得保存监听器指针
`emplaceListener` 返回的 `ListenerPtr` 需要保存（如类成员变量），否则监听器可能被回收。
:::
