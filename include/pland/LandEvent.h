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

    LDAPI Player& getPlayer() const;
};
class PlayerAskCreateLandAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    bool    mIs3DLand;

public:
    LDAPI constexpr explicit PlayerAskCreateLandAfterEvent(Player& player, bool is3DLand)
    : mPlayer(player),
      mIs3DLand(is3DLand) {}

    LDAPI Player& getPlayer() const;
    LDAPI bool    is3DLand() const;
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

    LDAPI Player&           getPlayer() const;
    LDAPI LandSelectorData* getLandSelectorData() const;
    LDAPI int&              getPrice() const;
};
class PlayerBuyLandAfterEvent final : public ll::event::Event {
protected:
    Player&     mPlayer;
    LandDataPtr mLandData;

public:
    LDAPI explicit PlayerBuyLandAfterEvent(Player& player, LandDataPtr landData)
    : mPlayer(player),
      mLandData(landData) {}

    LDAPI Player&     getPlayer() const;
    LDAPI LandDataPtr getLandData() const;
};


// 玩家 进入/离开 领地(LandScheduler)
class PlayerEnterLandEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    LDAPI constexpr explicit PlayerEnterLandEvent(Player& player, LandID landID) : mPlayer(player), mLandID(landID) {}

    LDAPI Player& getPlayer() const;
    LDAPI LandID  getLandID() const;
};
class PlayerLeaveLandEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    LDAPI constexpr explicit PlayerLeaveLandEvent(Player& player, LandID landID) : mPlayer(player), mLandID(landID) {}

    LDAPI Player& getPlayer() const;
    LDAPI LandID  getLandID() const;
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

    LDAPI Player&    getPlayer() const;
    LDAPI LandID     getLandID() const;
    LDAPI int const& getRefundPrice() const;
};
class PlayerDeleteLandAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    LDAPI constexpr explicit PlayerDeleteLandAfterEvent(Player& player, LandID landID)
    : mPlayer(player),
      mLandID(landID) {}

    LDAPI Player& getPlayer() const;
    LDAPI LandID  getLandID() const;
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

    LDAPI Player&      getPlayer() const;
    LDAPI UUIDs const& getTargetPlayer() const;
    LDAPI LandID       getLandID() const;
    LDAPI bool         isAdd() const;
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

    LDAPI Player&      getPlayer() const;
    LDAPI UUIDs const& getTargetPlayer() const;
    LDAPI LandID       getLandID() const;
    LDAPI bool         isAdd() const;
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

    LDAPI Player& getPlayer() const;
    LDAPI Player& getNewOwner() const;
    LDAPI LandID  getLandID() const;
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

    LDAPI Player& getPlayer() const;
    LDAPI Player& getNewOwner() const;
    LDAPI LandID  getLandID() const;
};


// 领地范围变动(LandBuyWithReSelectGui)
class LandRangeChangeBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&            mPlayer;      // 操作者
    LandDataPtr const& mLandData;    // 操作的领地数据
    LandPos const&     mNewRange;    // 新范围
    int const&         mNeedPay;     // 需要支付的价格
    int const&         mRefundPrice; // 需要退的价格

public:
    LDAPI LandRangeChangeBeforeEvent(
        Player&            player,
        LandDataPtr const& landData,
        LandPos const&     newRange,
        int const&         needPay,
        int const&         refundPrice
    )
    : Cancellable(),
      mPlayer(player),
      mLandData(landData),
      mNewRange(newRange),
      mNeedPay(needPay),
      mRefundPrice(refundPrice) {}

    LDAPI Player&            getPlayer() const;
    LDAPI LandDataPtr const& getLandData() const;
    LDAPI LandPos const&     getNewRange() const;
    LDAPI int const&         getNeedPay() const;
    LDAPI int const&         getRefundPrice() const;
};
class LandRangeChangeAfterEvent final : public ll::event::Event {
protected:
    Player&            mPlayer;      // 操作者
    LandDataPtr const& mLandData;    // 操作的领地数据
    LandPos const&     mNewRange;    // 新范围
    int const&         mNeedPay;     // 需要支付的价格
    int const&         mRefundPrice; // 需要退的价格

public:
    LDAPI LandRangeChangeAfterEvent(
        Player&            player,
        LandDataPtr const& landData,
        LandPos const&     newRange,
        int const&         needPay,
        int const&         refundPrice
    )
    : mPlayer(player),
      mLandData(landData),
      mNewRange(newRange),
      mNeedPay(needPay),
      mRefundPrice(refundPrice) {}

    LDAPI Player&            getPlayer() const;
    LDAPI LandDataPtr const& getLandData() const;
    LDAPI LandPos const&     getNewRange() const;
    LDAPI int const&         getNeedPay() const;
    LDAPI int const&         getRefundPrice() const;
};


} // namespace land