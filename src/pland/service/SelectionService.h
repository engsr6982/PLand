#pragma once
#include "pland/Global.h"

#include "ll/api/Expected.h"

#include <memory>

class Player;

namespace land {
class SelectorManager;
class AbstractSelector;
} // namespace land

namespace land::service {


class SelectionService {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    explicit SelectionService(SelectorManager& manager);
    ~SelectionService();

    /**
     * @return 玩家是否有正在进行中的选区任务
     */
    LDNDAPI bool hasActiveSelection(Player& player) const;

    /**
     * 开始一个新的选区任务
     */
    LDNDAPI ll::Expected<> beginSelection(Player& player, std::unique_ptr<AbstractSelector> selector);

    /**
     * 结束当前选区任务
     */
    LDAPI void endSelection(Player& player);

    /**
     * 获取当前选区任务
     */
    LDNDAPI AbstractSelector* tryGetSelection(Player& player) const;
};


} // namespace land::service