#include "pland/land/LandEvent.h"
#include "TestMain.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "pland/PLand.h"

namespace test {

std::vector<ll::event::ListenerPtr> mLandEventListeners;

void TestMain::_setupLandEventTest() {
    auto& bus    = ll::event::EventBus::getInstance();
    auto  logger = &land::PLand::getInstance().getSelf().getLogger();

    mLandEventListeners = {
        bus.emplaceListener<land::PlayerAskCreateLandBeforeEvent>([logger](land::PlayerAskCreateLandBeforeEvent& ev) {
            logger->debug("[Test] PlayerAskCreateLandBeforeEvent - Player: {}", ev.getPlayer().getRealName());
        }),
        bus.emplaceListener<land::PlayerAskCreateLandAfterEvent>([logger](land::PlayerAskCreateLandAfterEvent& ev) {
            logger->debug(
                "[Test] PlayerAskCreateLandAfterEvent - Player: {}, Is3D: {}",
                ev.getPlayer().getRealName(),
                ev.is3DLand()
            );
        }),
        bus.emplaceListener<land::PlayerBuyLandBeforeEvent>([logger](land::PlayerBuyLandBeforeEvent& ev) {
            logger->debug(
                "[Test] PlayerBuyLandBeforeEvent - Player: {}, Price: {}",
                ev.getPlayer().getRealName(),
                ev.getPrice()
            );
        }),
        bus.emplaceListener<land::PlayerBuyLandAfterEvent>([logger](land::PlayerBuyLandAfterEvent& ev) {
            logger->debug(
                "[Test] PlayerBuyLandAfterEvent - Player: {}, LandID: {}",
                ev.getPlayer().getRealName(),
                ev.getLand()->getId()
            );
        }),
        bus.emplaceListener<land::PlayerEnterLandEvent>([logger](land::PlayerEnterLandEvent& ev) {
            logger->debug(
                "[Test] PlayerEnterLandEvent - Player: {}, LandID: {}",
                ev.getPlayer().getRealName(),
                ev.getLandID()
            );
        }),
        bus.emplaceListener<land::PlayerLeaveLandEvent>([logger](land::PlayerLeaveLandEvent& ev) {
            logger->debug(
                "[Test] PlayerLeaveLandEvent - Player: {}, LandID: {}",
                ev.getPlayer().getRealName(),
                ev.getLandID()
            );
        }),
        bus.emplaceListener<land::PlayerDeleteLandBeforeEvent>([logger](land::PlayerDeleteLandBeforeEvent& ev) {
            logger->debug(
                "[Test] PlayerDeleteLandBeforeEvent - Player: {}, LandID: {}, RefundPrice: {}",
                ev.getPlayer().getRealName(),
                ev.getLandID(),
                ev.getRefundPrice()
            );
        }),
        bus.emplaceListener<land::PlayerDeleteLandAfterEvent>([logger](land::PlayerDeleteLandAfterEvent& ev) {
            logger->debug(
                "[Test] PlayerDeleteLandAfterEvent - Player: {}, LandID: {}",
                ev.getPlayer().getRealName(),
                ev.getLandID()
            );
        }),
        bus.emplaceListener<land::LandMemberChangeBeforeEvent>([logger](land::LandMemberChangeBeforeEvent& ev) {
            logger->debug(
                "[Test] LandMemberChangeBeforeEvent - Player: {}, LandID: {}, IsAdd: {}",
                ev.getPlayer().getRealName(),
                ev.getLandID(),
                ev.isAdd()
            );
        }),
        bus.emplaceListener<land::LandMemberChangeAfterEvent>([logger](land::LandMemberChangeAfterEvent& ev) {
            logger->debug(
                "[Test] LandMemberChangeAfterEvent - Player: {}, LandID: {}, IsAdd: {}",
                ev.getPlayer().getRealName(),
                ev.getLandID(),
                ev.isAdd()
            );
        }),
        bus.emplaceListener<land::LandOwnerChangeBeforeEvent>([logger](land::LandOwnerChangeBeforeEvent& ev) {
            logger->debug(
                "[Test] LandOwnerChangeBeforeEvent - Player: {}, NewOwner: {}, LandID: {}",
                ev.getPlayer().getRealName(),
                ev.getNewOwner().getRealName(),
                ev.getLandID()
            );
        }),
        bus.emplaceListener<land::LandOwnerChangeAfterEvent>([logger](land::LandOwnerChangeAfterEvent& ev) {
            logger->debug(
                "[Test] LandOwnerChangeAfterEvent - Player: {}, NewOwner: {}, LandID: {}",
                ev.getPlayer().getRealName(),
                ev.getNewOwner().getRealName(),
                ev.getLandID()
            );
        }),
        bus.emplaceListener<land::LandRangeChangeBeforeEvent>([logger](land::LandRangeChangeBeforeEvent& ev) {
            logger->debug(
                "[Test] LandRangeChangeBeforeEvent - Player: {}, LandID: {}, NeedPay: {}, RefundPrice: {}",
                ev.getPlayer().getRealName(),
                ev.getLand()->getId(),
                ev.getNeedPay(),
                ev.getRefundPrice()
            );
        }),
        bus.emplaceListener<land::LandRangeChangeAfterEvent>([logger](land::LandRangeChangeAfterEvent& ev) {
            logger->debug(
                "[Test] LandRangeChangeAfterEvent - Player: {}, LandID: {}, NeedPay: {}, RefundPrice: {}",
                ev.getPlayer().getRealName(),
                ev.getLand()->getId(),
                ev.getNeedPay(),
                ev.getRefundPrice()
            );
        }),
    };

    logger->debug("LandEventTest init success");
}

} // namespace test
