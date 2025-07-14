#include "LegacyMoneyInterface.h"
#include "ll/api/service/PlayerInfo.h"
#include "mc/world/actor/player/Player.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace land::internals {


static const wchar_t* LEGACY_MONEY_MODULE_NAME = L"LegacyMoney.dll";
#define THROW_LEGACY_MONEY_NOT_LOADED() throw std::runtime_error("LegacyMoney not loaded.");

// c function pointer
using LLMoney_Get_Func    = llong (*)(std::string);
using LLMoney_Set_Func    = bool (*)(std::string, llong);
using LLMoney_Add_Func    = bool (*)(std::string, llong);
using LLMoney_Reduce_Func = bool (*)(std::string, llong);
using LLMoney_Trans_Func  = bool (*)(std::string from, std::string to, llong val, const std::string& note);

LegacyMoneyInterface::LegacyMoneyInterface() = default;

std::optional<std::string> LegacyMoneyInterface::getXuidFromPlayerInfo(mce::UUID const& uuid) const {
    auto info = ll::service::PlayerInfo::getInstance().fromUuid(uuid);
    if (!info) {
        return std::nullopt;
    }
    return info->xuid;
}

bool LegacyMoneyInterface::isLegacyMoneyLoaded() const { return GetModuleHandle(LEGACY_MONEY_MODULE_NAME) != nullptr; }

long long LegacyMoneyInterface::get(Player& player) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }

    auto func = (LLMoney_Get_Func)GetProcAddress(GetModuleHandle(LEGACY_MONEY_MODULE_NAME), "LLMoney_Get");
    if (!func) {
        throw std::runtime_error("Dynamic call to LLMoney_Get failed.");
    }
    return func(player.getXuid());
}
long long LegacyMoneyInterface::get(mce::UUID const& uuid) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto xuid = getXuidFromPlayerInfo(uuid);
    if (!xuid) {
        return 0;
    }
    return get(*xuid);
}

bool LegacyMoneyInterface::set(Player& player, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto func = (LLMoney_Set_Func)GetProcAddress(GetModuleHandle(LEGACY_MONEY_MODULE_NAME), "LLMoney_Set");
    if (!func) {
        throw std::runtime_error("Dynamic call to LLMoney_Set failed.");
    }
    return func(player.getXuid(), amount);
}
bool LegacyMoneyInterface::set(mce::UUID const& uuid, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto xuid = getXuidFromPlayerInfo(uuid);
    if (!xuid) {
        return false;
    }
    return set(*xuid, amount);
}

bool LegacyMoneyInterface::add(Player& player, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto func = (LLMoney_Add_Func)GetProcAddress(GetModuleHandle(LEGACY_MONEY_MODULE_NAME), "LLMoney_Add");
    if (!func) {
        throw std::runtime_error("Dynamic call to LLMoney_Add failed.");
    }
    return func(player.getXuid(), amount);
}
bool LegacyMoneyInterface::add(mce::UUID const& uuid, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto xuid = getXuidFromPlayerInfo(uuid);
    if (!xuid) {
        return false;
    }
    return add(*xuid, amount);
}

bool LegacyMoneyInterface::reduce(Player& player, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto func = (LLMoney_Reduce_Func)GetProcAddress(GetModuleHandle(LEGACY_MONEY_MODULE_NAME), "LLMoney_Reduce");
    if (!func) {
        throw std::runtime_error("Dynamic call to LLMoney_Reduce failed.");
    }
    return func(player.getXuid(), amount);
}
bool LegacyMoneyInterface::reduce(mce::UUID const& uuid, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto xuid = getXuidFromPlayerInfo(uuid);
    if (!xuid) {
        return false;
    }
    return reduce(*xuid, amount);
}

bool LegacyMoneyInterface::transfer(Player& from, Player& to, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto func = (LLMoney_Trans_Func)GetProcAddress(GetModuleHandle(LEGACY_MONEY_MODULE_NAME), "LLMoney_Trans");
    if (!func) {
        throw std::runtime_error("Dynamic call to LLMoney_Trans failed.");
    }
    return func(from.getXuid(), to.getXuid(), amount, "PLand internal transfer");
}
bool LegacyMoneyInterface::transfer(mce::UUID const& from, mce::UUID const& to, long long amount) const {
    if (!isLegacyMoneyLoaded()) {
        THROW_LEGACY_MONEY_NOT_LOADED()
    }
    auto fromXuid = getXuidFromPlayerInfo(from);
    if (!fromXuid) {
        return false;
    }
    auto toXuid = getXuidFromPlayerInfo(to);
    if (!toXuid) {
        return false;
    }
    return transfer(*fromXuid, *toXuid, amount);
}
} // namespace land::internals