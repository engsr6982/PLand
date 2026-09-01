#include "pland/service/LeasingService.h"

#include "LandPriceService.h"
#include "SelectionService.h"

#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include <ll/api/service/Bedrock.h>

#include "pland/PLand.h"
#include "pland/economy/EconomySystem.h"
#include "pland/economy/PriceCalculate.h"
#include "pland/enums/LandHoldType.h"
#include "pland/enums/LeaseState.h"
#include "pland/events/domain/LandRecycleEvent.h"
#include "pland/events/domain/LandStateChangedEvent.h"
#include "pland/events/player/PlayerLeaseLandEvent.h"
#include "pland/events/player/PlayerMoveEvent.h"
#include "pland/events/player/PlayerRenewLandEvent.h"
#include "pland/infra/TimerScheduler.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/land/validator/LandCreateValidator.h"
#include "pland/selector/land/OrdinaryLandCreateSelector.h"
#include "pland/service/LandHierarchyService.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/SwitchExecutor.h"
#include "pland/utils/TimeUtils.h"

#include "mc/world/level/Level.h"

#include <chrono>

namespace land::service {

static_assert(sizeof(time_t) >= 8, "time_t must be 64-bit");

struct LeasingService::Impl {
    LandRegistry&     mRegistry;
    LandPriceService& mLandPriceService;
    SelectionService& mSelectionService;

    TimerScheduler<LandID> mScheduler;

    ll::event::ListenerPtr mPlayerJoinListener{nullptr};
    ll::event::ListenerPtr mPlayerEnterLandListener{nullptr};

    std::shared_ptr<std::atomic<bool>>            mQuit{nullptr};
    std::shared_ptr<ll::coro::InterruptableSleep> mSleep{nullptr};

    explicit Impl(LandRegistry& registry, LandPriceService& priceService, SelectionService& selectionService)
    : mRegistry(registry),
      mLandPriceService(priceService),
      mSelectionService(selectionService) {}


    ll::Expected<> enqueue(std::shared_ptr<Land> const& land) {
        assert(land);
        assert(land->isLeased());

        auto state = land->getLeaseState();
        if (state == LeaseState::Active) {
            mScheduler.push(land->getLeaseEndAt(), land->getId());
        } else if (state == LeaseState::Frozen) {
            // 如果已经是冻结状态，关注的是它何时被回收
            auto freezeDayTs = time_utils::toSeconds(ConfigProvider::getLeasingConfig().freeze.days);
            auto freezeEnd   = land->getLeaseEndAt() + freezeDayTs;

            // 只有死亡时间还在未来，才压入调度器。如果在过去，说明它该死但还没死，压入 now 让它下一秒立刻死。
            auto now = time_utils::nowSeconds();
            mScheduler.push(std::max(freezeEnd, now), land->getId());
        }
        return {};
    }
};

LeasingService::LeasingService(
    LandRegistry&     registry,
    LandPriceService& priceService,
    SelectionService& selectionService
)
: impl(std::make_unique<Impl>(registry, priceService, selectionService)) {
    auto const& conf = ConfigProvider::getLeasingConfig();
    if (!conf.enabled) {
        return; // 租赁系统未启用，资源无需初始化
    }

    auto& bus = ll::event::EventBus::getInstance();

    impl->mQuit  = std::make_shared<std::atomic<bool>>(false);
    impl->mSleep = std::make_shared<ll::coro::InterruptableSleep>();

    if (conf.notifications.loginTip)
        impl->mPlayerJoinListener =
            bus.emplaceListener<ll::event::PlayerJoinEvent>([this](ll::event::PlayerJoinEvent& ev) {
                auto& player = ev.self();
                if (player.isSimulatedPlayer()) return;

                auto uuid     = player.getUuid();
                auto threadId = std::this_thread::get_id();
                ll::coro::keepThis([uuid, registry = &impl->mRegistry, threadId]() -> ll::coro::CoroTask<> {
                    int frozen  = 0;
                    int nearDue = 0;

                    {
                        // 切换到线程池中扫描领地
                        co_await coro_utils::SwitchExecutor{PLand::getInstance().getThreadPool()};

                        assert(std::this_thread::get_id() != threadId);

                        auto now   = time_utils::nowSeconds();
                        auto lands = registry->getLands(uuid);

                        auto renewalAdvanceTs =
                            time_utils::toSeconds(ConfigProvider::getLeasingConfig().duration.renewalAdvance);
                        for (auto& land : lands) {
                            if (!land->isLeased()) continue;

                            bool isActuallyFrozen =
                                land->isLeaseFrozen()
                                || (land->getLeaseState() == LeaseState::Active && land->getLeaseEndAt() <= now);
                            if (isActuallyFrozen) {
                                frozen++;
                                continue;
                            }

                            auto remaining = land->getLeaseEndAt() - now;
                            if (remaining <= renewalAdvanceTs) {
                                nearDue++;
                            }
                        }
                    }

                    // 切回主线程
                    co_await ll::coro::yield;
                    assert(std::this_thread::get_id() == threadId);
                    if (auto player = ll::service::getLevel()->getPlayer(uuid)) {
                        if (frozen != 0) {
                            feedback_utils::sendText(
                                *player,
                                "您有 {} 个租赁领地已冻结，请尽快缴费续租!"_trl(player->getLocaleCode(), frozen)

                            );
                        }
                        if (nearDue != 0) {
                            feedback_utils::sendText(
                                *player,
                                "您有 {} 个租赁领地即将到期，请及时续租!"_trl(player->getLocaleCode(), nearDue)
                            );
                        }
                    }
                    co_return;
                }).launch(ll::thread::ServerThreadExecutor::getDefault());
            });

    if (conf.notifications.enterTip)
        impl->mPlayerEnterLandListener =
            bus.emplaceListener<event::PlayerEnterLandEvent>([this](event::PlayerEnterLandEvent& ev) {
                auto& player = ev.self();
                auto  land   = impl->mRegistry.getLand(ev.landId());
                if (!land || !land->isLeased() || !land->isOwner(player.getUuid())) {
                    return;
                }

                auto now = time_utils::nowSeconds();

                bool isActuallyFrozen = land->isLeaseFrozen()
                                     || (land->getLeaseState() == LeaseState::Active && land->getLeaseEndAt() <= now);
                if (isActuallyFrozen) {
                    auto days   = time_utils::ceilDays(now - land->getLeaseEndAt());
                    auto detail = impl->mLandPriceService.calculateRenewCost(land, days);
                    feedback_utils::sendText(player, "此领地已冻结，欠费 {}"_trl(player.getLocaleCode(), detail.total));
                    return;
                }

                auto remaining = land->getLeaseEndAt() - now;
                if (remaining <= time_utils::toSeconds(ConfigProvider::getLeasingConfig().duration.renewalAdvance)) {
                    auto localeCode = player.getLocaleCode();
                    feedback_utils::sendText(
                        player,
                        "此领地租赁即将到期：{}"_trl(localeCode, time_utils::formatDuration(remaining))
                    );
                }
            });

    // 初始化定时器
    for (auto& land : impl->mRegistry.getLands()) {
        if (land->isLeased()) {
            if (land->getLeaseState() == LeaseState::Active) {
                impl->mScheduler.push(land->getLeaseEndAt(), land->getId());
            } else if (land->getLeaseState() == LeaseState::Frozen) {
                impl->mScheduler.push(land->getLeaseEndAt() + time_utils::toSeconds(conf.freeze.days), land->getId());
            }
        }
    }

    ll::coro::keepThis([quit = impl->mQuit, sleep = impl->mSleep, this]() -> ll::coro::CoroTask<> {
        while (!quit->load()) {
            co_await sleep->sleepFor(std::chrono::seconds{5});
            if (quit->load()) {
                break;
            }

            auto now = time_utils::nowSeconds();

            auto const& conf = ConfigProvider::getLeasingConfig();

            auto freezeDayTs = time_utils::toSeconds(conf.freeze.days);

            auto dueLands = impl->mScheduler.popDueTasks(now);
            for (auto landId : dueLands) {
                auto land = impl->mRegistry.getLand(landId);
                if (!land || !land->isLeased()) continue;

                auto state = land->getLeaseState();

                if (state == LeaseState::Active) {
                    if (land->getLeaseEndAt() <= now) { // 懒惰双检，防止玩家刚才一瞬间续费了
                        land->setLeaseState(LeaseState::Frozen);

                        // 基于原本的到期时间计算死期, 而不是现在的 now
                        auto freezeEnd = land->getLeaseEndAt() + freezeDayTs;
                        if (freezeEnd <= now) {
                            // 如果因为服务器停机太久，开服时发现连冻结期都过了，直接回收
                            (void)recycleLand(land, LandRecycleReason::LeaseExpired);
                        } else {
                            // 正常推入死亡时间
                            impl->mScheduler.push(freezeEnd, landId);
                        }
                    }
                } else if (state == LeaseState::Frozen) {
                    auto freezeEnd = land->getLeaseEndAt() + freezeDayTs;
                    // 懒惰双检：过滤掉那些在冻结期被玩家续费了，但旧任务还在队列里的“幽灵任务”
                    if (freezeEnd <= now) {
                        (void)recycleLand(land, LandRecycleReason::LeaseExpired);
                    }
                }
            }
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

LeasingService::~LeasingService() {
    auto const& conf = ConfigProvider::getLeasingConfig();
    if (conf.enabled) {
        auto& bus = ll::event::EventBus::getInstance();
        bus.removeListener(impl->mPlayerJoinListener);
        bus.removeListener(impl->mPlayerEnterLandListener);
        impl->mQuit->store(true);
        impl->mSleep->interrupt(true);
    }
}

ll::Expected<> LeasingService::ensureLandLeased(std::shared_ptr<Land> const& land) const {
    if (!land) {
        return ll::makeStringError("land not found");
    }
    if (!land->isLeased()) {
        return ll::makeStringError("land is not in leased state");
    }

    // assert
    if (!land->isOrdinaryLand()) {
        PLand::getInstance().getSelf().getLogger().error(
            "Data inconsistency detected: leased land {} has parent/child land, "
            "this violates mutex condition",
            land->getId()
        );
        return ll::makeStringError("leased land has invalid hierarchical relationship");
    }

    return {};
}

bool LeasingService::enabled() const { return ConfigProvider::getLeasingConfig().enabled; }

void LeasingService::refreshSchedule(std::shared_ptr<Land> const& land) {
    if (land && land->isLeased()) {
        impl->enqueue(land); // 重新压入调度器
    }
}

ll::Expected<>
LeasingService::setStartAt(std::shared_ptr<Land> const& land, std::chrono::system_clock::time_point date) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }
    auto ts = time_utils::toSeconds(date);
    if (!time_utils::ensureTimestamp(ts)) {
        return ll::makeStringError("invalid timestamp");
    }
    if (ts >= land->getLeaseEndAt()) {
        return ll::makeStringError("lease start time must be earlier than end time");
    }
    land->setLeaseStartAt(ts);
    return {};
}

ll::Expected<> LeasingService::setEndAt(std::shared_ptr<Land> const& land, std::chrono::system_clock::time_point date) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }
    auto ts = time_utils::toSeconds(date);
    if (!time_utils::ensureTimestamp(ts)) {
        return ll::makeStringError("invalid timestamp");
    }
    if (ts <= land->getLeaseStartAt()) {
        return ll::makeStringError("lease end time must be later than start time");
    }

    auto now         = time_utils::nowSeconds();
    auto freezeDayTs = time_utils::toSeconds(ConfigProvider::getLeasingConfig().freeze.days);
    auto oldState    = land->getLeaseState();

    land->setLeaseEndAt(ts);

    // 推导状态
    if (ts > now) {
        // 结束时间在未来
        if (oldState != LeaseState::Active) {
            land->setLeaseState(LeaseState::Active);
        }
    } else if (ts + freezeDayTs > now) {
        // 已过期，但在冻结宽限期内
        if (oldState != LeaseState::Frozen) {
            land->setLeaseState(LeaseState::Frozen);
        }
    } else {
        // 连冻结期都过了，死透了
        if (oldState != LeaseState::Expired) {
            // 时间被修改为很久之前，无需调度，直接回收
            return recycleLand(land, LandRecycleReason::LeaseExpired);
        }
    }

    // 依然存活（Active/Frozen），重新排入调度系统
    if (land->getLeaseState() != LeaseState::Expired) {
        refreshSchedule(land);
    }
    return {};
}

ll::Expected<> LeasingService::forceFreeze(std::shared_ptr<Land> const& land) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }
    if (!land->isLeaseActive()) {
        return ll::makeStringError("Only territories that are in a normal (Active) state can be forcibly frozen.");
    }

    auto now = time_utils::nowSeconds();

    // 忽略剩余租期，立即结算
    if (land->getLeaseEndAt() > now) {
        land->setLeaseEndAt(now);
    }

    land->setLeaseState(LeaseState::Frozen);

    refreshSchedule(land); // 压入调度器，等待协程回收
    return {};
}

ll::Expected<> LeasingService::forceRecycle(std::shared_ptr<Land> const& land) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }
    return recycleLand(land, LandRecycleReason::ForceRecycle);
}

ll::Expected<> LeasingService::addTime(std::shared_ptr<Land> const& land, int seconds) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }
    auto newEnd = time_utils::toClockTime(land->getLeaseEndAt() + seconds);
    return setEndAt(land, newEnd);
}

ll::Expected<int> LeasingService::cleanExpiredLands(int daysOverdue) {
    if (daysOverdue < 0) {
        return ll::makeStringError("The overdue days must be a positive integer");
    }

    int  count       = 0;
    auto now         = time_utils::nowSeconds();
    auto freezeDayTs = time_utils::toSeconds(ConfigProvider::getLeasingConfig().freeze.days);
    auto threshold   = now - time_utils::toSeconds(daysOverdue);

    auto& logger = PLand::getInstance().getSelf().getLogger();

    std::vector<std::shared_ptr<Land>> toDelete;
    for (auto& land : impl->mRegistry.getLands(SYSTEM_ACCOUNT_UUID)) {
        if (!land->isLeased()) continue;

        if (!land->isOrdinaryLand()) {
            logger.fatal(
                "FATAL: Leased land {} has parent/child land, "
                "this violates mutex condition and CANNOT be auto-repaired. "
                "Manual intervention required. Land will be skipped in cleanup.",
                land->getId()
            );
            continue;
        }

        // 只清理已经被回收（Expired）且属于系统的领地
        if (land->isLeaseExpired()) {
            // 它的实际“死亡/被系统接管时间”是 LeaseEndAt + freezeDayTs
            auto deathTime = land->getLeaseEndAt() + freezeDayTs;

            if (deathTime <= threshold) {
                toDelete.push_back(land);
            }
        }
    }

    for (auto ptr : toDelete) {
        if (auto result = impl->mRegistry.removeOrdinaryLand(ptr)) {
            count++;
        } else {
            logger.error("Failed to remove expired land {} from registry: {}", ptr->getId(), result.error().message());
        }
    }

    return count;
}

ll::Expected<> LeasingService::toBought(std::shared_ptr<Land> const& land) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }

    land->setHoldType(LandHoldType::Bought); // 转为购买模式
    land->setLeaseState(LeaseState::None);   // 重置状态

    land->setLeaseStartAt(0);
    land->setLeaseEndAt(0);

    return {};
}

ll::Expected<> LeasingService::toLeased(std::shared_ptr<Land> const& land, int days) {
    if (!land) {
        return ll::makeStringError("land is null");
    }
    if (land->isLeased()) {
        return ll::makeStringError("land is already leased");
    }
    if (!land->isOrdinaryLand()) {
        return ll::makeStringError("only ordinary land can be leased");
    }

    land->setHoldType(LandHoldType::Leased);

    auto now = time_utils::nowSeconds();
    land->setLeaseStartAt(now);
    land->setLeaseEndAt(now + time_utils::toSeconds(days));
    land->setLeaseState(LeaseState::Active);

    refreshSchedule(land);
    return {};
}

ll::Expected<std::shared_ptr<Land>>
LeasingService::leaseLand(Player& player, OrdinaryLandCreateSelector* selector, int days) {
    if (!ConfigProvider::isLeasingEnabled()) {
        return ll::makeStringError("租赁模式未启用"_trl(player.getLocaleCode()));
    }
    if (!selector) {
        return ll::makeStringError("无效选区"_trl(player.getLocaleCode()));
    }
    if (!Config::ensureLeasingDimensionAllowed(selector->getDimensionId())) {
        return ll::makeStringError("你所在的维度无法租赁领地"_trl(player.getLocaleCode()));
    }

    auto const& duration = ConfigProvider::getLeasingConfig().duration;
    if (days < duration.minPeriod || days > duration.maxPeriod) {
        return ll::makeStringError("租赁天数不合法"_trl(player.getLocaleCode()));
    }

    auto range = selector->newLandAABB();
    if (!range) {
        return ll::makeStringError("选区范围无效"_trl(player.getLocaleCode()));
    }
    range->canonicalize();

    auto rentExp = impl->mLandPriceService.calculateDailyRent(*range, selector->getDimensionId(), selector->is3D());
    if (!rentExp) {
        PLand::getInstance().getSelf().getLogger().error(rentExp.error().message());
        return ll::makeStringError("价格表达式解析失败，请联系管理员"_trl(player.getLocaleCode()));
    }

    auto totalCost = rentExp.value() * static_cast<long long>(days);

    auto& economy = EconomySystem::getInstance();
    if (!economy.reduceChecked(player.getUuid(), totalCost)) {
        return ll::makeStringError("您的余额不足，无法租赁"_trl(player.getLocaleCode()));
    }

    auto land = selector->newLand();
    auto now  = time_utils::nowSeconds();

    assert(land);

    land->setHoldType(LandHoldType::Leased);
    land->setLeaseState(LeaseState::Active);
    land->setLeaseStartAt(now);
    land->setLeaseEndAt(now + time_utils::toSeconds(days));

    if (auto res = LandCreateValidator::validateCreateOrdinaryLand(impl->mRegistry, player, land); !res) {
        (void)economy->add(player.getUuid(), totalCost);
        return ll::forwardError(res.error());
    }

    if (auto res = impl->mRegistry.addOrdinaryLand(land); !res) {
        (void)economy->add(player.getUuid(), totalCost);
        return ll::makeStringError(res.error().message());
    }

    impl->mSelectionService.endSelection(player);

    ll::event::EventBus::getInstance().publish(event::PlayerLeaseLandEvent{player, land, totalCost, days});

    impl->enqueue(land);
    return land;
}

ll::Expected<> LeasingService::renewLease(Player& player, std::shared_ptr<Land> const& land, int days) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }

    auto const& duration = ConfigProvider::getLeasingConfig().duration;
    if (days < duration.minPeriod || days > duration.maxPeriod) {
        return ll::makeStringError("续租天数不合法"_trl(player.getLocaleCode()));
    }

    if (!land->isOwner(player.getUuid()) && !impl->mRegistry.isOperator(player.getUuid())) {
        return ll::makeStringError("操作失败，您不是领地主人"_trl(player.getLocaleCode()));
    }

    auto detail = impl->mLandPriceService.calculateRenewCost(land, days);
    if (detail.total <= 0) {
        return ll::makeStringError("租金计算失败"_trl(player.getLocaleCode()));
    }

    auto& economy = EconomySystem::getInstance();
    if (!economy.reduceChecked(player.getUuid(), detail.total)) {
        return ll::makeStringError("您的余额不足，无法续租"_trl(player.getLocaleCode()));
    }

    auto now     = time_utils::nowSeconds();
    auto endBase = land->getLeaseEndAt();
    if (endBase < now) {
        endBase = now;
    }

    land->setLeaseEndAt(endBase + time_utils::toSeconds(days));
    land->setLeaseState(LeaseState::Active);

    ll::event::EventBus::getInstance().publish(event::PlayerRenewLandEvent{player, land, detail.total, days});

    return impl->enqueue(land); // 玩家续租成功后，把新的时间推入调度器
}


ll::Expected<> LeasingService::recycleLand(std::shared_ptr<Land> const& land, LandRecycleReason reason) {
    if (auto expected = ensureLandLeased(land); !expected) {
        return expected;
    }

    auto const& conf = ConfigProvider::getLeasingConfig();

    auto now         = time_utils::nowSeconds();
    auto freezeDayTs = time_utils::toSeconds(conf.freeze.days);
    // 既然进入了 Expired (回收态)，它的数学定义就是“结束时间 + 冻结期 <= 现在”
    if (land->getLeaseEndAt() + freezeDayTs > now) {
        land->setLeaseEndAt(now - freezeDayTs);
    }

    switch (conf.recycle.mode) {
    case LeaseRecycleMode::TransferToSystem: {
        assert(SYSTEM_ACCOUNT_UUID != mce::UUID::EMPTY());
        assert(SYSTEM_ACCOUNT_UUID.asString() == SYSTEM_ACCOUNT_UUID_STR);
        if (!conf.recycle.keepMembers) {
            land->clearMembers();
        }
        land->setOwner(SYSTEM_ACCOUNT_UUID);
        land->setLeaseState(LeaseState::Expired);
        land->setName("[欠费|系统所有] {}"_tr(land->getName()));
        break;
    }
    case LeaseRecycleMode::Delete: {
        // todo: LandDeletedEvent
        if (auto result = impl->mRegistry.removeOrdinaryLand(land); !result) {
            return result;
        }
        break;
    }
    }

    ll::event::EventBus::getInstance().publish(event::LandRecycleEvent{land, reason});
    return {};
}


} // namespace land::service
