#pragma once
#include "EconomyConfig.h"
#include "pland/Global.h"

#include "econbridge/IEconomy.h"

#include <memory>
#include <mutex>


class Player;
namespace mce {
class UUID;
}

namespace land {


class EconomySystem final {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    LD_DISABLE_COPY_AND_MOVE(EconomySystem);
    explicit EconomySystem();

    LDNDAPI static EconomySystem& getInstance();

    LDAPI void initialize(); // 初始化经济系统
    LDAPI void reload();     // 重载经济系统（当 kit 改变时）

    LDNDAPI std::string getCostMessage(Player& player, llong amount) const;

    // 扣除玩家余额；扣款前显式校验余额是否充足，避免经济后端实现缺陷导致余额被扣成负数。
    // 返回 true 表示余额充足且已成功扣款。
    LDNDAPI bool reduceChecked(mce::UUID const& uuid, int64_t amount) const;

    LDNDAPI std::shared_ptr<econbridge::IEconomy> get() const;

    inline std::shared_ptr<econbridge::IEconomy> operator->() const { return get(); }
};


} // namespace land
