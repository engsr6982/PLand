#include "pland/LandEvent.h"
#include "ll/api/event/Emitter.h"
#include "mc/world/actor/player/Player.h"
#include "pland/LandData.h"


namespace land {

Player& PlayerAskCreateLandBeforeEvent::getPlayer() const { return mPlayer; }
Player& PlayerAskCreateLandAfterEvent::getPlayer() const { return mPlayer; }
bool    PlayerAskCreateLandAfterEvent::is3DLand() const { return mIs3DLand; }


Player&       PlayerBuyLandBeforeEvent::getPlayer() const { return mPlayer; }
Selector*     PlayerBuyLandBeforeEvent::getSelector() const { return mSelector; }
int&          PlayerBuyLandBeforeEvent::getPrice() const { return mPrice; }
Player&       PlayerBuyLandAfterEvent::getPlayer() const { return mPlayer; }
LandData_sptr PlayerBuyLandAfterEvent::getLandData() const { return mLandData; }


Player& PlayerEnterLandEvent::getPlayer() const { return mPlayer; }
LandID  PlayerEnterLandEvent::getLandID() const { return mLandID; }
Player& PlayerLeaveLandEvent::getPlayer() const { return mPlayer; }
LandID  PlayerLeaveLandEvent::getLandID() const { return mLandID; }


Player&    PlayerDeleteLandBeforeEvent::getPlayer() const { return mPlayer; }
LandID     PlayerDeleteLandBeforeEvent::getLandID() const { return mLandID; }
int const& PlayerDeleteLandBeforeEvent::getRefundPrice() const { return mRefundPrice; }
Player&    PlayerDeleteLandAfterEvent::getPlayer() const { return mPlayer; }
LandID     PlayerDeleteLandAfterEvent::getLandID() const { return mLandID; }


Player&      LandMemberChangeBeforeEvent::getPlayer() const { return mPlayer; }
UUIDs const& LandMemberChangeBeforeEvent::getTargetPlayer() const { return mTargetPlayer; }
LandID       LandMemberChangeBeforeEvent::getLandID() const { return mLandID; }
bool         LandMemberChangeBeforeEvent::isAdd() const { return mIsAdd; }
Player&      LandMemberChangeAfterEvent::getPlayer() const { return mPlayer; }
UUIDs const& LandMemberChangeAfterEvent::getTargetPlayer() const { return mTargetPlayer; }
LandID       LandMemberChangeAfterEvent::getLandID() const { return mLandID; }
bool         LandMemberChangeAfterEvent::isAdd() const { return mIsAdd; }


Player& LandOwnerChangeBeforeEvent::getPlayer() const { return mPlayer; }
Player& LandOwnerChangeBeforeEvent::getNewOwner() const { return mNewOwner; }
LandID  LandOwnerChangeBeforeEvent::getLandID() const { return mLandID; }
Player& LandOwnerChangeAfterEvent::getPlayer() const { return mPlayer; }
Player& LandOwnerChangeAfterEvent::getNewOwner() const { return mNewOwner; }
LandID  LandOwnerChangeAfterEvent::getLandID() const { return mLandID; }


Player&              LandRangeChangeBeforeEvent::getPlayer() const { return mPlayer; }
LandData_sptr const& LandRangeChangeBeforeEvent::getLandData() const { return mLandData; }
LandPos const&       LandRangeChangeBeforeEvent::getNewRange() const { return mNewRange; }
int const&           LandRangeChangeBeforeEvent::getNeedPay() const { return mNeedPay; }
int const&           LandRangeChangeBeforeEvent::getRefundPrice() const { return mRefundPrice; }
Player&              LandRangeChangeAfterEvent::getPlayer() const { return mPlayer; }
LandData_sptr const& LandRangeChangeAfterEvent::getLandData() const { return mLandData; }
LandPos const&       LandRangeChangeAfterEvent::getNewRange() const { return mNewRange; }
int const&           LandRangeChangeAfterEvent::getNeedPay() const { return mNeedPay; }
int const&           LandRangeChangeAfterEvent::getRefundPrice() const { return mRefundPrice; }


// EventEmitter
#define IMPLEMENT_EVENT_EMITTER(EventName)                                                                             \
    static std::unique_ptr<ll::event::EmitterBase> emitterFactory##EventName();                                        \
    class EventName##Emitter : public ll::event::Emitter<emitterFactory##EventName, EventName> {};                     \
    static std::unique_ptr<ll::event::EmitterBase> emitterFactory##EventName() {                                       \
        return std::make_unique<EventName##Emitter>();                                                                 \
    }

IMPLEMENT_EVENT_EMITTER(PlayerAskCreateLandBeforeEvent)
IMPLEMENT_EVENT_EMITTER(PlayerAskCreateLandAfterEvent)
IMPLEMENT_EVENT_EMITTER(PlayerBuyLandBeforeEvent)
IMPLEMENT_EVENT_EMITTER(PlayerBuyLandAfterEvent)
IMPLEMENT_EVENT_EMITTER(PlayerEnterLandEvent)
IMPLEMENT_EVENT_EMITTER(PlayerLeaveLandEvent)
IMPLEMENT_EVENT_EMITTER(PlayerDeleteLandBeforeEvent)
IMPLEMENT_EVENT_EMITTER(PlayerDeleteLandAfterEvent)
IMPLEMENT_EVENT_EMITTER(LandMemberChangeBeforeEvent)
IMPLEMENT_EVENT_EMITTER(LandMemberChangeAfterEvent)
IMPLEMENT_EVENT_EMITTER(LandOwnerChangeBeforeEvent)
IMPLEMENT_EVENT_EMITTER(LandOwnerChangeAfterEvent)
IMPLEMENT_EVENT_EMITTER(LandRangeChangeBeforeEvent)
IMPLEMENT_EVENT_EMITTER(LandRangeChangeAfterEvent)

} // namespace land