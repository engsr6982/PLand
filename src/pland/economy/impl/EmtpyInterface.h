#pragma once
#include "IEconomyInterface.h"


namespace land::internals {


/**
 * @brief 空的经济系统, 什么也不做，全返回成功，用于禁用经济系统
 */
class EmtpyInterface final : public IEconomyInterface {
public:
    LDAPI explicit EmtpyInterface();

public: // override
    LDNDAPI llong get(Player& player) const override;
    LDNDAPI llong get(mce::UUID const& uuid) const override;

    LDNDAPI bool set(Player& player, llong amount) const override;
    LDNDAPI bool set(mce::UUID const& uuid, llong amount) const override;

    LDNDAPI bool add(Player& player, llong amount) const override;
    LDNDAPI bool add(mce::UUID const& uuid, llong amount) const override;

    LDNDAPI bool reduce(Player& player, llong amount) const override;
    LDNDAPI bool reduce(mce::UUID const& uuid, llong amount) const override;

    LDNDAPI bool transfer(Player& from, Player& to, llong amount) const override;
    LDNDAPI bool transfer(mce::UUID const& from, mce::UUID const& to, llong amount) const override;
};


} // namespace land::internals