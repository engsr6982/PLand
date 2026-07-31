#include "ConfigMigrator.h"

#include "pland/drawer/DrawerType.h"

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

namespace land {
namespace internal {

ConfigMigrator::ConfigMigrator() {
    registerMigrationUnit(32, [](ConfigMigrator::container_t& json) -> bool {
        if (json.contains("land")) {
            auto& land = json["land"];
            if (land.contains("subLand")) {
                auto& subLand = land["subLand"];
                if (subLand.contains("minSpacingIncludeY")) {
                    auto& minSpacingIncludeY = subLand["minSpacingIncludeY"];
                    if (minSpacingIncludeY.is_number_integer()) {
                        // fix(land): correct minSpacingIncludeY type from int to bool
                        // https://github.com/IceBlcokMC/PLand/pull/183/changes/aeb1d99cc15723ad10f8ea687091aa4f48001d48
                        bool val           = minSpacingIncludeY.get<int>();
                        minSpacingIncludeY = val;
                    }
                }
            }
        }
        return true;
    });

    registerMigrationUnit(34, [](ConfigMigrator::container_t& root) -> bool {
        constexpr Route routes[] = {
            // selector
            {                         "selector.tool",                              "selector.item"},

            // features
            {                           "land.landTp",                      "features.landTeleport"},

            {                 "land.setupDrawCommand",                      "features.draw.enabled"},
            {                        "land.drawRange",                        "features.draw.range"},
            {                "land.drawHandleBackend",                      "features.draw.backend"},

            {                     "land.tip.enterTip",        "features.notifications.enterLandTip"},
            {           "land.tip.bottomContinuedTip", "features.notifications.bottomContinuousTip"},
            {           "land.tip.bottomTipFrequency",      "features.notifications.bottomTipCycle"},

            // constraints
            {                          "land.maxLand",              "constraints.maxLandsPerPlayer"},

            {                   "land.textRules.name",                       "constraints.nameRule"},

            {           "land.bought.squareRange.min",             "constraints.size.minSideLength"},
            {           "land.bought.squareRange.max",             "constraints.size.maxSideLength"},
            {     "land.bought.squareRange.minHeight",                 "constraints.size.minHeight"},

            {                       "land.minSpacing",            "constraints.spacing.minDistance"},
            {               "land.minSpacingIncludeY",               "constraints.spacing.includeY"},

            {           "land.bought.forbiddenRanges",                "constraints.forbiddenRanges"},

            // business
            {"land.bought.dimensionPriceCoefficients",        "business.dimensionalPriceMultiplier"},

            {                     "land.discountRate",                      "business.discountRate"},

            {                       "land.refundRate",                 "business.bought.refundRate"},
            {           "land.bought.allowDimensions",            "business.bought.allowDimensions"},

            {   "land.bought.threeDimensionl.enabled",             "business.bought.mode3D.enabled"},
            { "land.bought.threeDimensionl.calculate",             "business.bought.mode3D.formula"},
            {     "land.bought.twoDimensionl.enabled",             "business.bought.mode2D.enabled"},
            {   "land.bought.twoDimensionl.calculate",             "business.bought.mode2D.formula"},

            {                  "land.subLand.enabled",            "business.bought.subLand.enabled"},
            {                "land.subLand.maxNested",     "business.bought.subLand.maxNestedDepth"},
            {               "land.subLand.minSpacing",         "business.bought.subLand.minSpacing"},
            {       "land.subLand.minSpacingIncludeY", "business.bought.subLand.minSpacingIncludeY"},
            {               "land.subLand.maxSubLand", "business.bought.subLand.maxSubLandsPerLand"},
            {                "land.subLand.calculate",            "business.bought.subLand.formula"},

            // system
            {                              "internal",                                     "system"},
        };

        for (auto const& route : routes) {
            mapPath(root, route);
        }

        return true;
    });
}

ConfigMigrator& ConfigMigrator::getInstance() {
    static ConfigMigrator instance;
    return instance;
}

} // namespace internal
} // namespace land