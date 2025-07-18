#pragma once
#include "pland/Global.h"
#include "pland/economy/EconomyConfig.h"

namespace land::internals {


class IEconomyInterface {
public:
    LD_DISALLOW_COPY_AND_MOVE(IEconomyInterface);

    LDAPI virtual ~IEconomyInterface() = default;

    LDAPI explicit IEconomyInterface();

public:
    LDNDAPI virtual llong get(Player& player) const        = 0;
    LDNDAPI virtual llong get(mce::UUID const& uuid) const = 0;

    LDNDAPI virtual bool set(Player& player, llong amount) const        = 0;
    LDNDAPI virtual bool set(mce::UUID const& uuid, llong amount) const = 0;

    LDNDAPI virtual bool add(Player& player, llong amount) const        = 0;
    LDNDAPI virtual bool add(mce::UUID const& uuid, llong amount) const = 0;

    LDNDAPI virtual bool reduce(Player& player, llong amount) const        = 0;
    LDNDAPI virtual bool reduce(mce::UUID const& uuid, llong amount) const = 0;

    LDNDAPI virtual bool transfer(Player& from, Player& to, llong amount) const                   = 0;
    LDNDAPI virtual bool transfer(mce::UUID const& from, mce::UUID const& to, llong amount) const = 0;

    LDNDAPI virtual bool has(Player& player, llong amount) const;
    LDNDAPI virtual bool has(mce::UUID const& uuid, llong amount) const;

public:
    LDNDAPI virtual std::string getCostMessage(Player& player, llong amount) const;

    LDAPI virtual void sendNotEnoughMoneyMessage(Player& player, llong amount) const;

    LDNDAPI virtual EconomyConfig& getConfig() const;
};


} // namespace land::internals