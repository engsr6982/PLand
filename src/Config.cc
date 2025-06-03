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

    return status ? status : tryUpdate();
}

bool Config::trySave() {
    auto dir = mod::ModEntry::getInstance().getSelf().getConfigDir() / CONFIG_FILE;

    bool status = ll::config::saveConfig(cfg, dir);

    return status;
}

bool Config::tryUpdate() {
    if (Config::cfg.version < 16) { // 假设新版本是16，如果配置文件版本低于16，则更新
        // 添加默认的生物类型名称
        Config::cfg.mob.hostileMobTypeNames = {
            "minecraft:zombie",
            "minecraft:skeleton",
            "minecraft:creeper",
            "minecraft:spider",
            "minecraft:enderman",
            "minecraft:witch",
            "minecraft:blaze",
            "minecraft:ghast",
            "minecraft:magma_cube",
            "minecraft:silverfish",
            "minecraft:slime",
            "minecraft:zombified_piglin",
            "minecraft:wither_skeleton",
            "minecraft:stray",
            "minecraft:husk",
            "minecraft:drowned",
            "minecraft:phantom",
            "minecraft:pillager",
            "minecraft:vindicator",
            "minecraft:evoker",
            "minecraft:ravager",
            "minecraft:hoglin",
            "minecraft:zoglin",
            "minecraft:piglin_brute",
            "minecraft:shulker",
            "minecraft:guardian",
            "minecraft:elder_guardian",
            "minecraft:ender_dragon",
            "minecraft:wither",
            "minecraft:cave_spider",
            "minecraft:endermite",
            "minecraft:vex",
            "minecraft:piglin",
            "minecraft:goat",
            "minecraft:polar_bear",
            "minecraft:pufferfish",
            "minecraft:rabbit",
        };
        Config::cfg.mob.specialMobTypeNames = {
            "minecraft:armor_stand",
            "minecraft:item",
            "minecraft:xp_orb",
            "minecraft:painting",
            "minecraft:item_frame",
            "minecraft:leash_knot",
            "minecraft:boat",
            "minecraft:minecart",
            "minecraft:chest_minecart",
            "minecraft:furnace_minecart",
            "minecraft:tnt_minecart",
            "minecraft:hopper_minecart",
            "minecraft:command_block_minecart",
            "minecraft:ender_crystal",
            "minecraft:fireball",
            "minecraft:wither_skull",
            "minecraft:dragon_fireball",
            "minecraft:arrow",
            "minecraft:snowball",
            "minecraft:egg",
            "minecraft:experience_bottle",
            "minecraft:splash_potion",
            "minecraft:lingering_potion",
            "minecraft:trident",
            "minecraft:fishing_hook",
            "minecraft:lightning_bolt",
            "minecraft:eye_of_ender_signal",
            "minecraft:fireworks_rocket",
            "minecraft:llama_spit",
            "minecraft:area_effect_cloud",
            "minecraft:falling_block",
            "minecraft:tnt",
            "minecraft:xp_bottle",
            "minecraft:donkey",
            "minecraft:mule",
            "minecraft:horse",
            "minecraft:skeleton_horse",
            "minecraft:zombie_horse",
            "minecraft:strider",
            "minecraft:camel",
            "minecraft:sniffer",
            "minecraft:frog",
            "minecraft:allay",
            "minecraft:bee",
            "minecraft:axolotl",
            "minecraft:glow_squid",
            "minecraft:iron_golem",
            "minecraft:snow_golem",
            "minecraft:villager",
            "minecraft:wandering_trader",
            "minecraft:fox",
            "minecraft:dolphin",
            "minecraft:parrot",
            "minecraft:ocelot",
            "minecraft:cat",
            "minecraft:wolf",
            "minecraft:panda",
            "minecraft:turtle",
            "minecraft:tadpole",
        };
        Config::cfg.mob.passiveMobTypeNames = {
            "minecraft:cow",
            "minecraft:sheep",
            "minecraft:pig",
            "minecraft:chicken",
            "minecraft:squid",
            "minecraft:bat",
            "minecraft:mooshroom",
            "minecraft:salmon",
            "minecraft:cod",
            "minecraft:tropicalfish",
        };
        Config::cfg.version = 16; // 更新版本号
    }
    return trySave();
}

Config Config::cfg; // static

} // namespace land
