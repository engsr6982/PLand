# Event & 领地事件

::: tip 领地的事件分为 `Before` 和 `After` 两个阶段，`Before` 阶段为事件触发前，`After` 阶段为事件触发后。
:::

::: tip `Before` 事件可以通过 `ev.cancel()` 取消事件，`After` 事件无法取消。
:::

::: tip 命名规则: `事件名` + `Before` / `After` + `Event`  
例如: `PlayerAskCreateLandBeforeEvent`
:::

::: tip 事件触发顺序: `预检查` -> `Before` -> `处理内容` -> `After`
:::

::: warning 非必要请不要修改事件 `const` 修饰的成员，除非你知道你在做什么。
:::

## **监听事件示例:**

```cpp
#include "pland/event/PlayerMoveEvent.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/ListenerBase.h"

ll::event::ListenerPtr mEnterLandListener;

void setup() {
    mEnterLandListener = ll::event::EventBus::getInstance().emplaceListener<land::event::PlayerEnterLandEvent>([](land::event::PlayerEnterLandEvent const& ev) {
        // do something
    });
}
```
