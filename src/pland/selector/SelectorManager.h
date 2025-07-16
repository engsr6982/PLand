#pragma once
#include "ISelector.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/event/ListenerBase.h"
#include "pland/Global.h"
#include "pland/infra/Require.h"
#include <unordered_map>


namespace land {


/**
 * @brief 选区管理器
 * @note 由 ModEntry 管理 (RAII)
 */
class SelectorManager final {
    std::unordered_map<UUIDm, std::unique_ptr<ISelector>> mSelectors{};
    std::unordered_map<UUIDm, std::time_t>                mStabilization{};
    ll::event::ListenerPtr                                mListener{nullptr};
    std::shared_ptr<std::atomic<bool>>                    mCoroStop{nullptr};
    std::shared_ptr<ll::coro::InterruptableSleep>         mInterruptableSleep{nullptr};

    LDAPI bool startSelectionImpl(std::unique_ptr<ISelector> selector);

public:
    LD_DISALLOW_COPY_AND_MOVE(SelectorManager);

    explicit SelectorManager();
    ~SelectorManager();

    // 玩家是否正在选区 & 是否有选区任务
    LDNDAPI bool hasSelector(UUIDm const& uuid) const;
    LDNDAPI bool hasSelector(Player& player) const;

    // 获取选区任务
    LDNDAPI ISelector* getSelector(UUIDm const& uuid) const;
    LDNDAPI ISelector* getSelector(Player& player) const;

    // 开始选区
    template <typename T>
        requires std::is_base_of_v<ISelector, T> && std::is_final_v<T>
    bool startSelection(std::unique_ptr<T> selector) {
        return startSelectionImpl(std::move(selector));
    }

    // 停止选区
    LDAPI void stopSelection(UUIDm const& uuid);
    LDAPI void stopSelection(Player& player);

    using ForEachFunc = std::function<bool(UUIDm const&, ISelector*)>;
    LDAPI void forEach(ForEachFunc const& func) const;
};

LD_DECLARE_REQUIRE(SelectorManager);

} // namespace land