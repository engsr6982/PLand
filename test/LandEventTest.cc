#include "pland/LandEvent.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "pland/mod/ModEntry.h"

#include "LandEventTest.h"

namespace test {

// 事件监听器指针
ll::event::ListenerPtr mPlayerAskCreateLandBeforeEvent;
ll::event::ListenerPtr mPlayerAskCreateLandAfterEvent;
ll::event::ListenerPtr mPlayerBuyLandBeforeEvent;
ll::event::ListenerPtr mPlayerBuyLandAfterEvent;
ll::event::ListenerPtr mPlayerEnterLandEvent;
ll::event::ListenerPtr mPlayerLeaveLandEvent;
ll::event::ListenerPtr mPlayerDeleteLandBeforeEvent;
ll::event::ListenerPtr mPlayerDeleteLandAfterEvent;
ll::event::ListenerPtr mLandMemberChangeBeforeEvent;
ll::event::ListenerPtr mLandMemberChangeAfterEvent;
ll::event::ListenerPtr mLandOwnerChangeBeforeEvent;
ll::event::ListenerPtr mLandOwnerChangeAfterEvent;
ll::event::ListenerPtr mLandRangeChangeBeforeEvent;
ll::event::ListenerPtr mLandRangeChangeAfterEvent;

void SetupEventListener() {
    auto& bus    = ll::event::EventBus::getInstance();
    auto  logger = &mod::ModEntry::getInstance().getSelf().getLogger();

    // 创建领地事件
    mPlayerAskCreateLandBeforeEvent =
        bus.emplaceListener<land::PlayerAskCreateLandBeforeEvent>([logger](land::PlayerAskCreateLandBeforeEvent& ev) {
            logger->debug("[Test] PlayerAskCreateLandBeforeEvent - Player: {}", ev.getPlayer().getName());
        });

    mPlayerAskCreateLandAfterEvent =
        bus.emplaceListener<land::PlayerAskCreateLandAfterEvent>([logger](land::PlayerAskCreateLandAfterEvent& ev) {
            logger->debug(
                "[Test] PlayerAskCreateLandAfterEvent - Player: {}, Is3D: {}",
                ev.getPlayer().getName(),
                ev.is3DLand()
            );
        });

    // 购买领地事件
    mPlayerBuyLandBeforeEvent =
        bus.emplaceListener<land::PlayerBuyLandBeforeEvent>([logger](land::PlayerBuyLandBeforeEvent& ev) {
            logger->debug(
                "[Test] PlayerBuyLandBeforeEvent - Player: {}, Price: {}",
                ev.getPlayer().getName(),
                ev.getPrice()
            );
        });

    mPlayerBuyLandAfterEvent =
        bus.emplaceListener<land::PlayerBuyLandAfterEvent>([logger](land::PlayerBuyLandAfterEvent& ev) {
            logger->debug(
                "[Test] PlayerBuyLandAfterEvent - Player: {}, LandID: {}",
                ev.getPlayer().getName(),
                ev.getLandData()->getLandID()
            );
        });

    // 进入/离开领地事件
    mPlayerEnterLandEvent = bus.emplaceListener<land::PlayerEnterLandEvent>([logger](land::PlayerEnterLandEvent& ev) {
        logger->debug("[Test] PlayerEnterLandEvent - Player: {}, LandID: {}", ev.getPlayer().getName(), ev.getLandID());
    });

    mPlayerLeaveLandEvent = bus.emplaceListener<land::PlayerLeaveLandEvent>([logger](land::PlayerLeaveLandEvent& ev) {
        logger->debug("[Test] PlayerLeaveLandEvent - Player: {}, LandID: {}", ev.getPlayer().getName(), ev.getLandID());
    });

    // 删除领地事件
    mPlayerDeleteLandBeforeEvent =
        bus.emplaceListener<land::PlayerDeleteLandBeforeEvent>([logger](land::PlayerDeleteLandBeforeEvent& ev) {
            logger->debug(
                "[Test] PlayerDeleteLandBeforeEvent - Player: {}, LandID: {}, RefundPrice: {}",
                ev.getPlayer().getName(),
                ev.getLandID(),
                ev.getRefundPrice()
            );
        });

    mPlayerDeleteLandAfterEvent =
        bus.emplaceListener<land::PlayerDeleteLandAfterEvent>([logger](land::PlayerDeleteLandAfterEvent& ev) {
            logger->debug(
                "[Test] PlayerDeleteLandAfterEvent - Player: {}, LandID: {}",
                ev.getPlayer().getName(),
                ev.getLandID()
            );
        });

    // 成员变动事件
    mLandMemberChangeBeforeEvent =
        bus.emplaceListener<land::LandMemberChangeBeforeEvent>([logger](land::LandMemberChangeBeforeEvent& ev) {
            logger->debug(
                "[Test] LandMemberChangeBeforeEvent - Player: {}, LandID: {}, IsAdd: {}",
                ev.getPlayer().getName(),
                ev.getLandID(),
                ev.isAdd()
            );
        });

    mLandMemberChangeAfterEvent =
        bus.emplaceListener<land::LandMemberChangeAfterEvent>([logger](land::LandMemberChangeAfterEvent& ev) {
            logger->debug(
                "[Test] LandMemberChangeAfterEvent - Player: {}, LandID: {}, IsAdd: {}",
                ev.getPlayer().getName(),
                ev.getLandID(),
                ev.isAdd()
            );
        });

    // 所有者变动事件
    mLandOwnerChangeBeforeEvent =
        bus.emplaceListener<land::LandOwnerChangeBeforeEvent>([logger](land::LandOwnerChangeBeforeEvent& ev) {
            logger->debug(
                "[Test] LandOwnerChangeBeforeEvent - Player: {}, NewOwner: {}, LandID: {}",
                ev.getPlayer().getName(),
                ev.getNewOwner().getName(),
                ev.getLandID()
            );
        });

    mLandOwnerChangeAfterEvent =
        bus.emplaceListener<land::LandOwnerChangeAfterEvent>([logger](land::LandOwnerChangeAfterEvent& ev) {
            logger->debug(
                "[Test] LandOwnerChangeAfterEvent - Player: {}, NewOwner: {}, LandID: {}",
                ev.getPlayer().getName(),
                ev.getNewOwner().getName(),
                ev.getLandID()
            );
        });

    // 范围变动事件
    mLandRangeChangeBeforeEvent =
        bus.emplaceListener<land::LandRangeChangeBeforeEvent>([logger](land::LandRangeChangeBeforeEvent& ev) {
            logger->debug(
                "[Test] LandRangeChangeBeforeEvent - Player: {}, LandID: {}, NeedPay: {}, RefundPrice: {}",
                ev.getPlayer().getName(),
                ev.getLandData()->getLandID(),
                ev.getNeedPay(),
                ev.getRefundPrice()
            );
        });

    mLandRangeChangeAfterEvent =
        bus.emplaceListener<land::LandRangeChangeAfterEvent>([logger](land::LandRangeChangeAfterEvent& ev) {
            logger->debug(
                "[Test] LandRangeChangeAfterEvent - Player: {}, LandID: {}, NeedPay: {}, RefundPrice: {}",
                ev.getPlayer().getName(),
                ev.getLandData()->getLandID(),
                ev.getNeedPay(),
                ev.getRefundPrice()
            );
        });

    logger->debug("LandEventTest init success");
}

} // namespace test
