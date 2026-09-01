#include "EventInterceptor.h"

#include "InterceptorConfig.h"

#include <ll/api/event/EventBus.h>

#include <absl/container/flat_hash_map.h>

namespace land::internal::interceptor {

struct EventInterceptor::Impl {
    absl::flat_hash_map<bool InterceptorConfig::Listeners::*, ll::event::ListenerPtr>  mListeners;
    absl::flat_hash_map<bool InterceptorConfig::Hooks::*, std::unique_ptr<IHookGuard>> mHookGuards;
};

EventInterceptor::EventInterceptor() : impl(std::make_unique<Impl>()) { reload(); }
EventInterceptor::~EventInterceptor() {
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& [_, listener] : impl->mListeners) {
        bus.removeListener(listener);
    }
    impl->mListeners.clear();
    impl->mHookGuards.clear(); // RAII
}

void EventInterceptor::reload() {
    setupLLPlayerListeners();
    setupLLEntityListeners();
    setupLLWorldListeners();
    setupIlaPlayerListeners();
    setupIlaEntityListeners();
    setupIlaWorldListeners();
    setupHooks();
}

bool EventInterceptor::isListenerAlreadyRegistered(bool InterceptorConfig::Listeners::* configure) const {
    auto iter = impl->mListeners.find(configure);
    return iter != impl->mListeners.end() && iter->second != nullptr;
}
bool EventInterceptor::isHookAlreadyRegistered(bool InterceptorConfig::Hooks::* configure) const {
    auto iter = impl->mHookGuards.find(configure);
    return iter != impl->mHookGuards.end() && iter->second != nullptr;
}

void EventInterceptor::_registerListener(
    bool InterceptorConfig::Listeners::* configure,
    ll::event::ListenerPtr               listener
) {
    if (!listener) return;
    impl->mListeners.insert_or_assign(configure, std::move(listener));
}
void EventInterceptor::_registerHook(
    bool InterceptorConfig::Hooks::* configure,
    std::unique_ptr<IHookGuard>      hookGuard
) {
    if (!hookGuard) return;
    impl->mHookGuards.insert_or_assign(configure, std::move(hookGuard));
}
void EventInterceptor::_unregisterListener(bool InterceptorConfig::Listeners::* configure) {
    auto iter = impl->mListeners.find(configure);
    if (iter != impl->mListeners.end()) {
        ll::event::EventBus::getInstance().removeListener(iter->second);
        impl->mListeners.erase(iter);
    }
}
void EventInterceptor::_unregisterHook(bool InterceptorConfig::Hooks::* configure) {
    auto iter = impl->mHookGuards.find(configure);
    if (iter != impl->mHookGuards.end()) {
        impl->mHookGuards.erase(iter);
    }
}

} // namespace land::internal::interceptor