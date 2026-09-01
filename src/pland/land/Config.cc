#include "pland/land/Config.h"
#include "internal/ConfigMigrator.h"
#include "pland/utils/JsonUtil.h"

#include "ll/api/Config.h"
#include "ll/api/Expected.h"
#include "ll/api/io/FileUtils.h"

#include <filesystem>
#include <string>

#include "fmt/format.h"

namespace land {

namespace fs = std::filesystem;

decltype(ConfigProvider::cfg) ConfigProvider::cfg = {};

std::filesystem::path ConfigProvider::_filePath(const std::filesystem::path& baseDir) { return baseDir / FILE_NAME; }
ll::Expected<>        ConfigProvider::load(const std::filesystem::path& baseDir) {
    auto path = _filePath(baseDir);

    if (!fs::exists(path)) {
        return save(baseDir);
    }

    auto data = ll::file_utils::readFile(path);
    if (!data) {
        return ll::makeStringError("Failed to read config file: " + path.string());
    }

    try {
        auto  json     = json_util::json_t::parse(*data);
        auto& migrator = internal::ConfigMigrator::getInstance();

        auto res = migrator.migrate(json, Impl::SchemaVersion);
        if (!res) {
            return ll::forwardError(res.error());
        }

        auto mres = json_util::merge_versioned_and_deserialize(json, cfg, true);

        // update migrated or merged config to disk
        if (res.value() == infra::MigrateResult::Success || mres == json_util::MergeResult::Modified) {
            ll::file_utils::writeFile(path, json.dump(4));
        }
    } catch (std::exception const& e) {
        return ll::makeStringError(fmt::format("Failed to parse config file: {}, error: {}", path.string(), e.what()));
    }
    return {};
}

ll::Expected<> ConfigProvider::save(const std::filesystem::path& baseDir) {
    auto path = _filePath(baseDir);
    ll::config::saveConfig(cfg, path);
    return {};
}

bool Config::ensureDimensionAllowed(int dimensionId) {
    return ConfigProvider::isDimensionAllowedForPurchase(dimensionId);
}

bool Config::ensureSubLandFeatureEnabled() { return ConfigProvider::isSubLandEnabled(); }
bool Config::ensureOrdinaryLandEnabled(bool is3D) { return ConfigProvider::isPurchaseModeEnabled(is3D); }
bool Config::ensureLeasingEnabled() { return ConfigProvider::isLeasingEnabled(); }
bool Config::ensureLeasingDimensionAllowed(int dimensionId) {
    return ConfigProvider::isDimensionAllowedForLeasing(dimensionId);
}
std::string const& Config::getLandPriceCalculateFormula(bool is3D) {
    return ConfigProvider::getPurchasePriceFormula(is3D);
}
std::string const& Config::getSubLandPriceCalculateFormula() { return ConfigProvider::getSubLandPriceFormula(); }

} // namespace land
