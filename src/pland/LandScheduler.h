#pragma once
#include "ll/api/event/ListenerBase.h"
#include "pland/Global.h"
#include "pland/Require.h"
#include <unordered_map>


namespace land {


/**
 * @brief 领地调度器
 * 处理玩家进出领地事件、以及标题提示等
 * 该类使用 RAII 管理资源，由 pland 的 ModEntry 持有
 */
class LandScheduler {
public:
    std::unordered_map<UUIDm, LandDimid> mDimidMap;
    std::unordered_map<UUIDm, LandID>    mLandidMap;
    ll::event::ListenerPtr               mPlayerEnterLandListener;

    LD_DISALLOW_COPY(LandScheduler);

    LDAPI explicit LandScheduler();
    LDAPI ~LandScheduler();
};


LD_DECLARE_REQUIRE(LandScheduler);


} // namespace land
