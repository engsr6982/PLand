#pragma once
#include "IEconomyInterface.h"
#include "pland/economy/impl/IEconomyInterface.h"

namespace land ::internals {


class ScoreBoardInterface final : public IEconomyInterface {
public:
    LDAPI explicit ScoreBoardInterface();

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
