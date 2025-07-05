#pragma once
#include "ll/api/event/ListenerBase.h"
#include "pland/Global.h"


namespace land {


/**
 * @brief EventListener
 * 领地事件监听器集合，RAII 管理事件监听器的注册和注销。
 */
class EventListener {
    std::vector<ll::event::ListenerPtr> mListenerPtrs;

    inline void RegisterListenerIf(bool need, std::function<ll::event::ListenerPtr()> const& factory);

public:
    LD_DISALLOW_COPY(EventListener);

    LDAPI explicit EventListener();
    LDAPI ~EventListener();
};


} // namespace land
