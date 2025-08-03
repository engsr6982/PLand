#include "pland/infra/Config.h"
#include "ll/api/Config.h"
#include "pland/PLand.h"
#include <filesystem>
#include <string>
#include <unordered_map>


namespace land {

namespace fs = std::filesystem;
#define CONFIG_FILE "Config.json"

bool Config::tryLoad() {
    auto dir = land::PLand::getInstance().getSelf().getConfigDir() / CONFIG_FILE;

    if (!fs::exists(dir)) {
        trySave();
    }

    bool status = ll::config::loadConfig(Config::cfg, dir);

    return status ? status : trySave();
}

bool Config::trySave() {
    auto dir = land::PLand::getInstance().getSelf().getConfigDir() / CONFIG_FILE;

    bool status = ll::config::saveConfig(cfg, dir);

    return status;
}


Config Config::cfg = [] {
    Config c;
    c.protection.permissionMaps.itemSpecific = {
        {          "minecraft:skull", "allowPlace"},
        {         "minecraft:banner", "allowPlace"},
        {   "minecraft:glow_ink_sac", "allowPlace"},
        {    "minecraft:end_crystal", "allowPlace"},
        {      "minecraft:ender_eye", "allowPlace"},
        {"minecraft:flint_and_steel", "useFlintAndSteel"},
        {      "minecraft:bone_meal", "useBoneMeal"},
        {    "minecraft:armor_stand", "allowPlace"}
    };
    c.protection.permissionMaps.blockSpecific = {
        {                "minecraft:dragon_egg", "allowAttackDragonEgg"},
        {                       "minecraft:bed", "useBed"},
        {                     "minecraft:chest", "allowOpenChest"},
        {             "minecraft:trapped_chest", "allowOpenChest"},
        {                  "minecraft:campfire", "useCampfire"},
        {             "minecraft:soul_campfire", "useCampfire"},
        {                 "minecraft:composter", "useComposter"},
        {                 "minecraft:noteblock", "useNoteBlock"},
        {                   "minecraft:jukebox", "useJukebox"},
        {                      "minecraft:bell", "useBell"},
        {"minecraft:daylight_detector_inverted", "useDaylightDetector"},
        {         "minecraft:daylight_detector", "useDaylightDetector"},
        {                   "minecraft:lectern", "useLectern"},
        {                  "minecraft:cauldron", "useCauldron"},
        {            "minecraft:respawn_anchor", "useRespawnAnchor"},
        {                "minecraft:flower_pot", "editFlowerPot"},
        {          "minecraft:sweet_berry_bush", "allowDestroy"}
    };
    c.protection.permissionMaps.blockFunctional = {
        {   "minecraft:cartography_table", "useCartographyTable"},
        {      "minecraft:smithing_table", "useSmithingTable"},
        {       "minecraft:brewing_stand", "useBrewingStand"},
        {               "minecraft:anvil", "useAnvil"},
        {          "minecraft:grindstone", "useGrindstone"},
        {    "minecraft:enchanting_table", "useEnchantingTable"},
        {              "minecraft:barrel", "useBarrel"},
        {              "minecraft:beacon", "useBeacon"},
        {              "minecraft:hopper", "useHopper"},
        {             "minecraft:dropper", "useDropper"},
        {           "minecraft:dispenser", "useDispenser"},
        {                "minecraft:loom", "useLoom"},
        {   "minecraft:stonecutter_block", "useStonecutter"},
        {             "minecraft:crafter", "useCrafter"},
        {  "minecraft:chiseled_bookshelf", "useChiseledBookshelf"},
        {                "minecraft:cake", "useCake"},
        {"minecraft:unpowered_comparator", "useComparator"},
        {  "minecraft:powered_comparator", "useComparator"},
        {  "minecraft:unpowered_repeater", "useRepeater"},
        {    "minecraft:powered_repeater", "useRepeater"},
        {            "minecraft:bee_nest", "useBeeNest"},
        {             "minecraft:beehive", "useBeeNest"},
        {               "minecraft:vault", "useVault"}
    };
    return c;
}();

} // namespace land
