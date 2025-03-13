#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
#include "pland/LandSelector.h"

namespace land {


// 玩家请求创建领地 (ChooseLandDimAndNewLand)
class PlayerAskCreateLandBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player& mPlayer;

public:
    LDAPI constexpr explicit PlayerAskCreateLandBeforeEvent(Player& player) : Cancellable(), mPlayer(player) {}

    LDNDAPI Player& getPlayer() const;
};
class PlayerAskCreateLandAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    bool    mIs3DLand;

public:
    LDAPI constexpr explicit PlayerAskCreateLandAfterEvent(Player& player, bool is3DLand)
    : mPlayer(player),
      mIs3DLand(is3DLand) {}

    LDNDAPI Player& getPlayer() const;
    LDNDAPI bool    is3DLand() const;
};


// 玩家购买领地 (LandBuyGui)
class PlayerBuyLandBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&           mPlayer;
    LandSelectorData* mLandSelectorData;
    int&              mPrice;

public:
    LDAPI constexpr explicit PlayerBuyLandBeforeEvent(Player& player, LandSelectorData* landSelectorData, int& price)
    : Cancellable(),
      mPlayer(player),
      mLandSelectorData(landSelectorData),
      mPrice(price) {}

    LDNDAPI Player&           getPlayer() const;
    LDNDAPI LandSelectorData* getLandSelectorData() const;
    LDNDAPI int&              getPrice() const;
};
class PlayerBuyLandAfterEvent final : public ll::event::Event {
protected:
    Player&       mPlayer;
    LandData_sptr mLandData;

public:
    LDAPI explicit PlayerBuyLandAfterEvent(Player& player, LandData_sptr landData)
    : mPlayer(player),
      mLandData(landData) {}

    LDNDAPI Player&       getPlayer() const;
    LDNDAPI LandData_sptr getLandData() const;
};


// 玩家 进入/离开 领地(LandScheduler)
class PlayerEnterLandEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    LDAPI constexpr explicit PlayerEnterLandEvent(Player& player, LandID landID) : mPlayer(player), mLandID(landID) {}

    LDNDAPI Player& getPlayer() const;
    LDNDAPI LandID  getLandID() const;
};
class PlayerLeaveLandEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    LDAPI constexpr explicit PlayerLeaveLandEvent(Player& player, LandID landID) : mPlayer(player), mLandID(landID) {}

    LDNDAPI Player& getPlayer() const;
    LDNDAPI LandID  getLandID() const;
};


// 玩家删除领地 (DeleteLandGui)
class PlayerDeleteLandBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&    mPlayer;
    LandID     mLandID;
    int const& mRefundPrice; // 删除后返还的金额

public:
    LDAPI constexpr explicit PlayerDeleteLandBeforeEvent(Player& player, LandID landID, int const& refundPrice)
    : Cancellable(),
      mPlayer(player),
      mLandID(landID),
      mRefundPrice(refundPrice) {}

    LDNDAPI Player&    getPlayer() const;
    LDNDAPI LandID     getLandID() const;
    LDNDAPI int const& getRefundPrice() const;
};
class PlayerDeleteLandAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    LDAPI constexpr explicit PlayerDeleteLandAfterEvent(Player& player, LandID landID)
    : mPlayer(player),
      mLandID(landID) {}

    LDNDAPI Player& getPlayer() const;
    LDNDAPI LandID  getLandID() const;
};


// 领地成员变动(EditLandMemberGui)
class LandMemberChangeBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&      mPlayer;       // 操作者
    UUIDs const& mTargetPlayer; // 目标玩家
    LandID       mLandID;
    bool         mIsAdd; // true: 添加成员, false: 删除成员

public:
    LDAPI LandMemberChangeBeforeEvent(Player& player, UUIDs const& targetPlayer, LandID landID, bool isAdd)
    : Cancellable(),
      mPlayer(player),
      mTargetPlayer(targetPlayer),
      mLandID(landID),
      mIsAdd(isAdd) {}

    LDNDAPI Player&      getPlayer() const;
    LDNDAPI UUIDs const& getTargetPlayer() const;
    LDNDAPI LandID       getLandID() const;
    LDNDAPI bool         isAdd() const;
};
class LandMemberChangeAfterEvent final : public ll::event::Event {
protected:
    Player&      mPlayer;       // 操作者
    UUIDs const& mTargetPlayer; // 目标玩家
    LandID       mLandID;
    bool         mIsAdd; // true: 添加成员, false: 删除成员

public:
    LDAPI LandMemberChangeAfterEvent(Player& player, UUIDs const& targetPlayer, LandID landID, bool isAdd)
    : mPlayer(player),
      mTargetPlayer(targetPlayer),
      mLandID(landID),
      mIsAdd(isAdd) {}

    LDNDAPI Player&      getPlayer() const;
    LDNDAPI UUIDs const& getTargetPlayer() const;
    LDNDAPI LandID       getLandID() const;
    LDNDAPI bool         isAdd() const;
};


// 领地主人变动(EditLandOwnerGui)
class LandOwnerChangeBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player& mPlayer;   // 操作者
    Player& mNewOwner; // 新主人
    LandID  mLandID;

public:
    LDAPI LandOwnerChangeBeforeEvent(Player& player, Player& newOwner, LandID landID)
    : Cancellable(),
      mPlayer(player),
      mNewOwner(newOwner),
      mLandID(landID) {}

    LDNDAPI Player& getPlayer() const;
    LDNDAPI Player& getNewOwner() const;
    LDNDAPI LandID  getLandID() const;
};
class LandOwnerChangeAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;   // 操作者
    Player& mNewOwner; // 目标玩家
    LandID  mLandID;

public:
    LDAPI LandOwnerChangeAfterEvent(Player& player, Player& newOwner, LandID landID)
    : mPlayer(player),
      mNewOwner(newOwner),
      mLandID(landID) {}

    LDNDAPI Player& getPlayer() const;
    LDNDAPI Player& getNewOwner() const;
    LDNDAPI LandID  getLandID() const;
};


// 领地范围变动(LandBuyWithReSelectGui)
class LandRangeChangeBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&              mPlayer;      // 操作者
    LandData_sptr const& mLandData;    // 操作的领地数据
    LandPos const&       mNewRange;    // 新范围
    int const&           mNeedPay;     // 需要支付的价格
    int const&           mRefundPrice; // 需要退的价格

public:
    LDAPI LandRangeChangeBeforeEvent(
        Player&              player,
        LandData_sptr const& landData,
        LandPos const&       newRange,
        int const&           needPay,
        int const&           refundPrice
    )
    : Cancellable(),
      mPlayer(player),
      mLandData(landData),
      mNewRange(newRange),
      mNeedPay(needPay),
      mRefundPrice(refundPrice) {}

    LDNDAPI Player&              getPlayer() const;
    LDNDAPI LandData_sptr const& getLandData() const;
    LDNDAPI LandPos const&       getNewRange() const;
    LDNDAPI int const&           getNeedPay() const;
    LDNDAPI int const&           getRefundPrice() const;
};
class LandRangeChangeAfterEvent final : public ll::event::Event {
protected:
    Player&              mPlayer;      // 操作者
    LandData_sptr const& mLandData;    // 操作的领地数据
    LandPos const&       mNewRange;    // 新范围
    int const&           mNeedPay;     // 需要支付的价格
    int const&           mRefundPrice; // 需要退的价格

public:
    LDAPI LandRangeChangeAfterEvent(
        Player&              player,
        LandData_sptr const& landData,
        LandPos const&       newRange,
        int const&           needPay,
        int const&           refundPrice
    )
    : mPlayer(player),
      mLandData(landData),
      mNewRange(newRange),
      mNeedPay(needPay),
      mRefundPrice(refundPrice) {}

    LDNDAPI Player&              getPlayer() const;
    LDNDAPI LandData_sptr const& getLandData() const;
    LDNDAPI LandPos const&       getNewRange() const;
    LDNDAPI int const&           getNeedPay() const;
    LDNDAPI int const&           getRefundPrice() const;
};


} // namespace land