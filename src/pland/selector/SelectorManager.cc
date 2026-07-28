#include "SelectorManager.h"
#include "AbstractSelector.h"
#include "pland/PLand.h"

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"

#include "mc/platform/UUID.h"
#include "mc/world/actor/player/Player.h"

#include <atomic>
#include <chrono>
#include <memory>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>


namespace land {

class CooldownLock {
    ll::coro::Duration                            mCooldown;
    std::shared_ptr<ll::coro::InterruptableSleep> mInterruptableSleep{nullptr};
    std::shared_ptr<std::atomic<bool>>            mAbortFlag{nullptr};
    ll::thread::ServerThreadExecutor const&       mExecutor;
    absl::flat_hash_set<mce::UUID>                mCoolingPlayers;

public:
    explicit CooldownLock(ll::coro::Duration cooldown, ll::thread::ServerThreadExecutor const& exec)
    : mCooldown(cooldown),
      mInterruptableSleep(std::make_shared<ll::coro::InterruptableSleep>()),
      mAbortFlag(std::make_shared<std::atomic<bool>>(false)),
      mExecutor(exec) {}

    ~CooldownLock() {
        mAbortFlag->store(true);
        mInterruptableSleep->interrupt(true); // inplace
    }

    bool tryTrigger(mce::UUID const& uuid) {
        auto [_, inserted] = mCoolingPlayers.insert(uuid);
        if (!inserted) {
            return false;
        }

        ll::coro::keepThis([this, uuid, sleep = mInterruptableSleep, abort = mAbortFlag]() -> ll::coro::CoroTask<> {
            co_await sleep->sleepFor(mCooldown);
            if (abort->load()) {
                co_return;
            }
            this->mCoolingPlayers.erase(uuid);
        }).launch(mExecutor);
        return true;
    }
};


struct SelectorManager::Impl {
    absl::flat_hash_map<mce::UUID, std::unique_ptr<AbstractSelector>> mSelectors{};

    std::unique_ptr<CooldownLock> mDebouncer{nullptr};

    ll::event::ListenerPtr                        mListener{nullptr};
    std::shared_ptr<std::atomic<bool>>            mCoroStop{nullptr};
    std::shared_ptr<ll::coro::InterruptableSleep> mInterruptableSleep{nullptr};

    ll::event::ListenerPtr mPlayerDisconnectListener;
};

SelectorManager::SelectorManager() : impl(std::make_unique<Impl>()) {
    impl->mDebouncer =
        std::make_unique<CooldownLock>(std::chrono::milliseconds(200), ll::thread::ServerThreadExecutor::getDefault());

    impl->mListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerInteractBlockEvent>(
        [this](ll::event::PlayerInteractBlockEvent const& ev) {
            auto& player = ev.self();

            if (player.isSimulatedPlayer() || !hasSelector(player)) {
                return;
            }

            auto selector = getSelector(player);
            if (!selector) {
                return;
            }

            if (!selector->isSpecifiedSelectTool(ev.item().getTypeName())) {
                return;
            }

            if (impl->mDebouncer->tryTrigger(player.getUuid())) {
                selector->selectNext(player, ev.blockPos());
            }
        }
    );

    impl->mPlayerDisconnectListener =
        ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
            [this](ll::event::PlayerDisconnectEvent const& ev) { this->stopSelection(ev.self()); }
        );

    impl->mCoroStop           = std::make_shared<std::atomic<bool>>(false);
    impl->mInterruptableSleep = std::make_shared<ll::coro::InterruptableSleep>();
    ll::coro::keepThis([sleep = impl->mInterruptableSleep, stop = impl->mCoroStop, this]() -> ll::coro::CoroTask<> {
        while (!stop->load()) {
            co_await sleep->sleepFor(ll::chrono::ticks(20));
            if (stop->load()) {
                break;
            }

            auto iter = impl->mSelectors.begin();
            while (iter != impl->mSelectors.end()) {
                auto& selector = iter->second;

                try {
                    selector->tick();
                    ++iter;
                } catch (std::exception const& e) {
                    impl->mSelectors.erase(iter++);
                    land::PLand::getInstance().getSelf().getLogger().error(
                        "SelectorManager: Exception in selector tick: {}",
                        e.what()
                    );
                } catch (...) {
                    impl->mSelectors.erase(iter++);
                    land::PLand::getInstance().getSelf().getLogger().error(
                        "SelectorManager: Unknown exception in selector tick"
                    );
                }
            }
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

SelectorManager::~SelectorManager() {
    ll::event::EventBus::getInstance().removeListener(impl->mListener);
    ll::event::EventBus::getInstance().removeListener(impl->mPlayerDisconnectListener);

    impl->mSelectors.clear();
    impl->mCoroStop->store(true);
    impl->mInterruptableSleep->interrupt(true);
}


bool SelectorManager::hasSelector(mce::UUID const& uuid) const { return impl->mSelectors.contains(uuid); }
bool SelectorManager::hasSelector(Player& player) const { return impl->mSelectors.contains(player.getUuid()); }

AbstractSelector* SelectorManager::getSelector(mce::UUID const& uuid) const {
    if (auto it = impl->mSelectors.find(uuid); it != impl->mSelectors.end()) {
        return it->second.get();
    }
    return nullptr;
}
AbstractSelector* SelectorManager::getSelector(Player& player) const { return getSelector(player.getUuid()); }

bool SelectorManager::startSelection(std::unique_ptr<AbstractSelector> selector) {
    auto& uuid = selector->getPlayer()->getUuid();
    if (hasSelector(uuid)) {
        return false;
    }
    return impl->mSelectors.insert({uuid, std::move(selector)}).second;
}

void SelectorManager::stopSelection(mce::UUID const& uuid) {
    if (auto it = impl->mSelectors.find(uuid); it != impl->mSelectors.end()) {
        impl->mSelectors.erase(it);
    }
}
void SelectorManager::stopSelection(Player& player) { return stopSelection(player.getUuid()); }

void SelectorManager::forEach(ForEachFunc const& func) const {
    for (auto const& [uuid, selector] : impl->mSelectors) {
        bool isContinue = func(uuid, selector.get());
        if (!isContinue) {
            break;
        }
    }
}

} // namespace land