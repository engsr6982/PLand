
#include "pland/hooks/EventListener.h"
#include "ll/api/event/EventBus.h"
#include <functional>

namespace land {

EventListener::EventListener() {
    // 调用所有分类的注册函数
    registerSessionListeners();
    registerPlayerListeners();
    registerEntityListeners();
    registerWorldListeners();
}

EventListener::~EventListener() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& ptr : mListenerPtrs) {
        bus.removeListener(ptr);
    }
}

void EventListener::RegisterListenerIf(bool need, std::function<ll::event::ListenerPtr()> const& factory) {
    if (need) {
        auto listenerPtr = factory();
        mListenerPtrs.push_back(std::move(listenerPtr));
    }
}

} // namespace land
