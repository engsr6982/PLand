#pragma once
#include "EconomyConfig.h"
#include "pland/Global.h"
#include "pland/economy/impl/IEconomyInterface.h"
#include <memory>
#include <mutex>
#include <string>


class Player;
namespace mce {
class UUID;
}

namespace land {


class EconomySystem final {
    std::shared_ptr<internals::IEconomyInterface> mEconomySystem;
    mutable std::mutex                            mInstanceMutex;

    explicit EconomySystem();

    void initEconomySystem();   // 初始化经济系统
    void reloadEconomySystem(); // 重载经济系统（当 kit 改变时）


public:
    LD_DISALLOW_COPY_AND_MOVE(EconomySystem);

    LDNDAPI static EconomySystem& getInstance();

    LDNDAPI EconomyConfig& getConfig() const;

    LDNDAPI std::shared_ptr<internals::IEconomyInterface> getEconomyInterface() const;

    LDNDAPI std::shared_ptr<internals::IEconomyInterface> operator->() const;

private:
    std::shared_ptr<internals::IEconomyInterface> createEconomySystem() const;
};


} // namespace land
