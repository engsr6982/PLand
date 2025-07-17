#include "pland/Global.h"
#include "mc/world/actor/player/Player.h"
#include "pland/PLand.h"
#include "pland/land/LandRegistry.h"
#include <unordered_map>


namespace land {

std::unordered_map<std::string, std::string> GlobalPlayerLocaleCodeCached;

std::string GetPlayerLocaleCodeFromSettings(Player& player) {
    auto uuid = player.getUuid().asString();
    auto iter = GlobalPlayerLocaleCodeCached.find(uuid);
    if (iter != GlobalPlayerLocaleCodeCached.end()) {
        return iter->second; // 命中缓存
    }

    if (auto set = PLand::getInstance().getLandRegistry()->getPlayerSettings(uuid); set) {
        if (set->localeCode == PlayerSettings::SYSTEM_LOCALE_CODE()) {
            GlobalPlayerLocaleCodeCached[uuid] = player.getLocaleCode();
        } else if (set->localeCode == PlayerSettings::SERVER_LOCALE_CODE()) {
            GlobalPlayerLocaleCodeCached[uuid] = std::string(ll::i18n::getDefaultLocaleCode());
        } else {
            GlobalPlayerLocaleCodeCached[uuid] = set->localeCode;
        }
    } else {
        GlobalPlayerLocaleCodeCached[uuid] = std::string(ll::i18n::getDefaultLocaleCode());
    }

    return GlobalPlayerLocaleCodeCached[uuid];
}

} // namespace land