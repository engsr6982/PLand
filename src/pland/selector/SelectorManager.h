#pragma once
#include "pland/Global.h"

class Player;

namespace land {

class AbstractSelector;

/**
 * @brief 选区管理器
 */
class SelectorManager final {
    struct Impl;
    std::unique_ptr<Impl> impl{nullptr};

public:
    LD_DISABLE_COPY_AND_MOVE(SelectorManager);

    explicit SelectorManager();
    ~SelectorManager();

    // 玩家是否正在选区 & 是否有选区任务
    LDNDAPI bool hasSelector(mce::UUID const& uuid) const;
    LDNDAPI bool hasSelector(Player& player) const;

    // 获取选区任务
    LDNDAPI AbstractSelector* getSelector(mce::UUID const& uuid) const;
    LDNDAPI AbstractSelector* getSelector(Player& player) const;

    // 开始选区
    LDNDAPI bool startSelection(std::unique_ptr<AbstractSelector> selector);

    // 停止选区
    LDAPI void stopSelection(mce::UUID const& uuid);
    LDAPI void stopSelection(Player& player);

    using ForEachFunc = std::function<bool(mce::UUID const&, AbstractSelector*)>;
    LDAPI void forEach(ForEachFunc const& func) const;
};


} // namespace land