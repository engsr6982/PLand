#include "Telemetry.h"
#include "pland/BuildInfo.h"

#include <ll/api/Versions.h>
#include <ll/api/service/Bedrock.h>

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <magic_enum.hpp>


namespace land::internal {
namespace adapter {

Telemetry::Telemetry() : ll_bstats::Telemetry(27389, std::string{BuildInfo::kBuildTag}) {}

void Telemetry::initConstant() {
    ll_bstats::Telemetry::initConstant();

    payload.addCustomChart(bstats::bukkit::SimplePie{"levilamina_version", {ll::getLoaderVersion().to_string()}});
}
void Telemetry::collect() {
    ll_bstats::Telemetry::collect();

    {
        std::unordered_map<std::string, int> platforms;
        ll::service::getLevel().transform([&platforms](auto& level) {
            level.forEachPlayer([&platforms](Player& player) {
                std::string platformName = std::string{magic_enum::enum_name(player.mBuildPlatform)};
                if (platforms.find(platformName) == platforms.end()) {
                    platforms.emplace(platformName, 1);
                } else {
                    platforms[platformName] += 1;
                }
                return true;
            });
            return true;
        });

        auto& charts = payload.getCustomCharts();
        auto  iter   = std::find_if(charts.begin(), charts.end(), [](auto& chart) {
            if (std::holds_alternative<bstats::bukkit::AdvancedPie>(chart)) {
                auto& pie = std::get<bstats::bukkit::AdvancedPie>(chart);
                return pie.chartId == "player_platform";
            }
            return false;
        });
        if (iter == charts.end()) {
            payload.addCustomChart(bstats::bukkit::AdvancedPie{"player_platform", {std::move(platforms)}});
        } else {
            auto& pie       = std::get<bstats::bukkit::AdvancedPie>(*iter);
            pie.data.values = std::move(platforms); // update
        }
    }
}


} // namespace adapter
} // namespace land::internal
