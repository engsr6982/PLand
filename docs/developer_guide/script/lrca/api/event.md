# LDEvent 脚本事件系统

:::warning 此接口相对于其它接口较为特殊，如果不使用 PLand-LRCA 提供的脚本封装，您需要自行处理事件监听器的注册
:::

### 生成脚本事件监听器 ID

`ScriptEventManager_genListenerID()`

- 参数: 无
- 返回值: 监听器 ID 字符串（配合 `Event_RegisterListener` 使用）
- 返回值类型: `String`

:::warning 一个监听器 ID 只能注册一个监听器，复用会导致未定义行为
:::

### 注册事件监听器

`Event_RegisterListener(eventName, scriptEventID)`

- 参数:
  - eventName : `String`
    事件名（见下方支持的事件列表）
  - scriptEventID : `String`
    由 `ScriptEventManager_genListenerID()` 生成，且回调已通过 `ll.exports(callback, eventName, scriptEventID)` 导出
- 返回值: 是否注册成功（事件名无效或回调未导出返回 `false`）
- 返回值类型: `Boolean`

:::warning 注意
无论事件是否可拦截，监听回调**都必须返回一个布尔值**，否则 RemoteCall 会抛出 `bad_variant_access`。  
`true` = 放行，`false` = 拦截（仅对可取消事件生效）。
:::

### 支持的事件列表

| 事件名                                                       | 回调参数                                                                                 |
| ------------------------------------------------------------ | ---------------------------------------------------------------------------------------- |
| `LandResizedEvent`                                           | `(landId, min, max)` — 领地范围调整（min/max 为 `IntPos`）                               |
| `MemberChangedEvent`                                         | `(landId, target, isAdd)` — 成员变更                                                     |
| `OwnerChangedEvent`                                          | `(landId, oldOwner, newOwner)` — 主人变更                                                |
| `LandRefundFailedEvent`                                      | `(landId, target, amount)` — 退款失败                                                    |
| `PlayerApplyLandRangeChangeBeforeEvent`                      | `(player, landId, min, max, type, newTotalPrice, amount)` — 执行变更领地范围前           |
| `PlayerApplyLandRangeChangeAfterEvent`                       | `(player, landId, min, max, type, newTotalPrice, amount)` — 执行变更领地范围后           |
| `PlayerBuyLandBeforeEvent`                                   | `(player, payMoney, landType)` — 购买领地前（可拦截）                                    |
| `PlayerBuyLandAfterEvent`                                    | `(player, landId, payMoney)` — 购买领地后                                                |
| `PlayerChangeLandMemberBeforeEvent`                          | `(player, landId, target, isAdd)` — 变更成员前（可拦截）                                 |
| `PlayerChangeLandMemberAfterEvent`                           | `(player, landId, target, isAdd)` — 变更成员后                                           |
| `PlayerChangeLandNameBeforeEvent`                            | `(player, landId, newName)` — 改名前（可拦截）                                           |
| `PlayerChangeLandNameAfterEvent`                             | `(player, landId, newName)` — 改名后                                                     |
| `PlayerDeleteLandBeforeEvent`                                | `(player, landId)` — 删除领地前（可拦截）                                                |
| `PlayerDeleteLandAfterEvent`                                 | `(player, landId)` — 删除领地后                                                          |
| `PlayerEnterLandEvent`                                       | `(player, landId)` — 进入领地                                                            |
| `PlayerLeaveLandEvent`                                       | `(player, landId)` — 离开领地                                                            |
| `PlayerRequestChangeLandRangeBeforeEvent`                    | `(player, landId)` — 请求变更范围前（可拦截）                                            |
| `PlayerRequestChangeLandRangeAfterEvent`                     | `(player, landId)` — 请求变更范围后                                                      |
| `PlayerRequestCreateLandEvent`                               | `(player, landType)` — 请求创建领地（可拦截）                                            |
| `PlayerTransferLandBeforeEvent`                              | `(player, landId, newOwner)` — 转让前（可拦截）                                          |
| `PlayerTransferLandAfterEvent`                               | `(player, landId, newOwner)` — 转让后                                                    |
| `LandRecycleEvent` <Badge type="tip" text="v0.19.0+" />      | `(landId, reason)` — 领地回收（reason：`0`=租赁到期 / `1`=闲置 / `2`=强制回收）          |
| `LandStateChangedEvent` <Badge type="tip" text="v0.19.0+" /> | `(landId, oldState, newState)` — 租赁状态变更（`0`=无 / `1`=正常 / `2`=冻结 / `3`=到期） |
| `MembersClearedEvent` <Badge type="tip" text="v0.19.0+" />   | `(landId)` — 成员被清空                                                                  |
| `PlayerLeaseLandEvent` <Badge type="tip" text="v0.19.0+" />  | `(landId, payMoney, days)` — 玩家租赁领地                                                |
| `PlayerRenewLandEvent` <Badge type="tip" text="v0.19.0+" />  | `(landId, payMoney, days)` — 玩家续租                                                    |

其中 `type` 为领地范围调整结算类型：`"NoChange"` / `"Pay"` / `"Refund"`；  
`landType` 为领地类型名：`"Ordinary"` / `"Parent"` / `"Mix"` / `"Sub"`。

### 手动注册事件示例

以下示例将演示如何手动注册一个`LandResizedEvent`事件监听器：

```js
// 监听回调函数，参数必须与事件参数一致
function onLandResizedEvent(landId, min, max) {
  // 处理事件逻辑, 此处省略

  return true;
}

// 辅助函数
function listenPLandEvent(eventName, callback) {
  // 导入事件监听器 ID 分配函数
  const allocId = ll.imports("PLand_LDAPI", "ScriptEventManager_genListenerID");

  // 获取一个监听器 ID (注意一个监听器 ID 只能使用一次，无论注册是否成功)
  const id = allocId();

  // 导出回调函数
  ll.exports(callback, eventName, id);

  // 向 PLand-LRCA 注册此监听器
  const register = ll.imports("PLand_LDAPI", "Event_RegisterListener");
  const ok = register(eventName, id);

  // 你可以选择 !ok 时抛出异常，或者返回给上层处理，或静默失败
  // 这里采用抛出异常处理
  if (!ok) {
    throw new Error(`Failed to register listener: ${eventName}`);
  }
}

// 注册函数
function setup() {
  listenPLandEvent("LandResizedEvent", onLandResizedEvent);
}

mc.listen("onServerStarted", () => {
  setup(); // 在服务器启动完成后注册事件监听器
});
```
