
#pragma once
#include "ll/api/event/ListenerBase.h"
#include "pland/Global.h"
#include <functional>
#include <vector>


namespace land {


/**
 * @brief EventListener
 * 领地事件监听器集合，RAII 管理事件监听器的注册和注销。
 */
class EventListener {
    std::vector<ll::event::ListenerPtr> mListenerPtrs;

    void RegisterListenerIf(bool need, std::function<ll::event::ListenerPtr()> const& factory);

    // 为不同事件类别声明注册函数
    void registerSessionListeners();
    void registerPlayerListeners();
    void registerEntityListeners();
    void registerWorldListeners();

public:
    LD_DISALLOW_COPY(EventListener);

    LDAPI explicit EventListener();
    LDAPI ~EventListener();
};


} // namespace land
