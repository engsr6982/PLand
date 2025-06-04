#include "PLand/Config.h"
#include "ll/api/Config.h"
#include "mod/ModEntry.h"
#include <filesystem>


namespace land {

namespace fs = std::filesystem;
#define CONFIG_FILE "Config.json"

bool Config::tryLoad() {
    auto dir = mod::ModEntry::getInstance().getSelf().getConfigDir() / CONFIG_FILE;

    if (!fs::exists(dir)) {
        trySave();
    }

    bool status = ll::config::loadConfig(Config::cfg, dir);

    return status ? status : trySave();
}

bool Config::trySave() {
    auto dir = mod::ModEntry::getInstance().getSelf().getConfigDir() / CONFIG_FILE;

    bool status = ll::config::saveConfig(cfg, dir);

    return status;
}


Config Config::cfg; // static

} // namespace land
