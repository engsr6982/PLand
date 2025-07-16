#include "SelectorManager.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/world/actor/player/Player.h"
#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/infra/Require.h"
#include "pland/utils/Date.h"
#include "pland/utils/McUtils.h"
#include <atomic>


namespace land {

LD_IMPL_REQUIRE(SelectorManager) {
    auto manager = land::PLand::getInstance().getSelectorManager();
    if (!manager) [[unlikely]] {
        throw std::runtime_error(LD_ERR_RAII_RESOURCE_EMPTY(SelectorManager));
    }
    return manager;
}


SelectorManager::SelectorManager() {
    mListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerInteractBlockEvent>(
        [this](ll::event::PlayerInteractBlockEvent const& ev) {
            auto& player = ev.self();

            if (player.isSimulatedPlayer() || !hasSelector(player)) {
                return;
            }

            // 防抖
            if (auto iter = mStabilization.find(player.getUuid()); iter != mStabilization.end()) {
                if (iter->second >= Date::now().getTime()) {
                    return;
                }
            }
            mStabilization[player.getUuid()] = Date::future(50 / 1000).getTime(); // 50ms

            if (ev.item().getTypeName() != Config::cfg.selector.tool) {
                return;
            }

            auto selector = getSelector(player);
            if (!selector) {
                return;
            }

            if (selector->isPointABSet()) {
                mc_utils::executeCommand("pland buy", &player); // TODO: 优化
                return;
            }

            if (!selector->isPointASet()) {
                selector->setPointA(ev.blockPos());
            } else if (!selector->isPointBSet()) {
                selector->setPointB(ev.blockPos());
            }
        }
    );


    mCoroStop           = std::make_shared<std::atomic<bool>>(false);
    mInterruptableSleep = std::make_shared<ll::coro::InterruptableSleep>();
    ll::coro::keepThis([sleep = mInterruptableSleep, stop = mCoroStop, this]() -> ll::coro::CoroTask<> {
        while (!stop->load()) {
            co_await sleep->sleepFor(ll::chrono::ticks(20));
            if (stop->load()) {
                break;
            }

            auto iter = mSelectors.begin();
            while (iter != mSelectors.end()) {
                auto& selector = iter->second;

                try {
                    if (!selector->getPlayer()) {
                        iter = mSelectors.erase(iter); // 玩家下线
                        continue;
                    }

                    selector->tick();

                    ++iter;
                } catch (std::exception const& e) {
                    iter = mSelectors.erase(iter);
                    land::PLand::getInstance().getSelf().getLogger().error(
                        "SelectorManager: Exception in selector tick: {}",
                        e.what()
                    );
                } catch (...) {
                    iter = mSelectors.erase(iter);
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
    mSelectors.clear();
    mCoroStop->store(true);
    mInterruptableSleep->interrupt(true);
}


bool SelectorManager::hasSelector(UUIDm const& uuid) const { return mSelectors.contains(uuid); }
bool SelectorManager::hasSelector(Player& player) const { return mSelectors.contains(player.getUuid()); }

ISelector* SelectorManager::getSelector(UUIDm const& uuid) const {
    if (auto it = mSelectors.find(uuid); it != mSelectors.end()) {
        return it->second.get();
    }
    return nullptr;
}
ISelector* SelectorManager::getSelector(Player& player) const { return getSelector(player.getUuid()); }

bool SelectorManager::startSelectionImpl(std::unique_ptr<ISelector> selector) {
    auto uuid = selector->getPlayer()->getUuid();
    if (hasSelector(uuid)) {
        return false;
    }
    return mSelectors.insert({uuid, std::move(selector)}).second;
}

void SelectorManager::stopSelection(UUIDm const& uuid) {
    if (auto it = mSelectors.find(uuid); it != mSelectors.end()) {
        mSelectors.erase(it);
    }
}
void SelectorManager::stopSelection(Player& player) { return stopSelection(player.getUuid()); }

void SelectorManager::forEach(ForEachFunc const& func) const {
    for (auto const& [uuid, selector] : mSelectors) {
        bool isContinue = func(uuid, selector.get());
        if (!isContinue) {
            break;
        }
    }
}

} // namespace land