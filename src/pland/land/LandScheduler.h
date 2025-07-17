#pragma once
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/event/ListenerBase.h"
#include "pland/Global.h"
#include <unordered_map>

class Player;

namespace land {


/**
 * @brief 领地调度器
 * 处理玩家进出领地事件、以及标题提示等
 * 该类使用 RAII 管理资源，由 pland 的 PLand 持有
 */
class LandScheduler {
private:
    std::vector<Player*>                   mPlayers{};
    std::unordered_map<Player*, LandDimid> mDimensionMap{};
    std::unordered_map<Player*, LandID>    mLandIdMap{};

    ll::event::ListenerPtr mPlayerJoinServerListener{nullptr};
    ll::event::ListenerPtr mPlayerDisconnectListener{nullptr};
    ll::event::ListenerPtr mPlayerEnterLandListener{nullptr};

    std::shared_ptr<std::atomic<bool>>            mQuit{nullptr};
    std::shared_ptr<ll::coro::InterruptableSleep> mEventSchedulingSleep{nullptr};
    std::shared_ptr<ll::coro::InterruptableSleep> mLandTipSchedulingSleep{nullptr};


public:
    LD_DISALLOW_COPY_AND_MOVE(LandScheduler);
    LDAPI explicit LandScheduler();
    LDAPI ~LandScheduler();

    LDAPI void tickEvent();
    LDAPI void tickLandTip();
};


} // namespace land
