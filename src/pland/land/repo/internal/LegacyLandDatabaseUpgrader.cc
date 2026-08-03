#include "LegacyLandDatabaseUpgrader.h"

#include "LandDatabase.h"
#include "LandMigrator.h"
#include "LegacyLandDatabaseReader.h"

#include "pland/utils/JsonUtil.h"

#include "ll/api/data/KeyValueDB.h"

#include <memory>

namespace land::internal {

bool LegacyLandDatabaseUpgrader::isLegacyDatabase(int32_t databaseVersion) {
    return databaseVersion <= kLegacyBoundary;
}

ll::Expected<>
LegacyLandDatabaseUpgrader::upgrade(std::unique_ptr<ll::data::KeyValueDB> legacyDatabase, LandDatabase& newDatabase) {
    auto reader = LegacyLandDatabaseReader{std::move(legacyDatabase)};

    auto legacyVersion = reader.getVersion();
    if (!isLegacyDatabase(legacyVersion)) {
        return ll::makeStringError(
            fmt::format(
                "Unsupported legacy database version: {}. Expected version: >={}",
                legacyVersion,
                kLegacyBoundary
            )
        );
    }

    auto updateFormatAndSave = [&](std::string_view key, std::string_view value) {
        auto j      = nlohmann::json::parse(value);
        auto buffer = nlohmann::json::to_cbor(j);
        auto strBuf = std::string{buffer.begin(), buffer.end()};
        return newDatabase.saveBinary(key, std::move(strBuf));
    };

    if (auto rawAdmins = reader.getAdmins()) {
        if (!updateFormatAndSave(LandDatabase::kAdminsKey, *rawAdmins)) {
            return ll::makeStringError("Failed to save admins data");
        }
    }
    if (auto rawSettings = reader.getPlayerSettings()) {
        if (!updateFormatAndSave(LandDatabase::kPlayerSettingsKey, *rawSettings)) {
            return ll::makeStringError("Failed to save player settings data");
        }
    }
    if (auto rawTemplatePerm = reader.getTemplatePerm()) {
        if (!updateFormatAndSave(LandDatabase::kTemplatePermTableKey, *rawTemplatePerm)) {
            return ll::makeStringError("Failed to save template permissions data");
        }
    }

    auto iter = reader.iterLands();
    for (auto [id, raw] : iter) {
        if (!updateFormatAndSave(LandDatabase::buildLandContextKey(id), raw)) {
            return ll::makeStringError(fmt::format("Failed to save land data for land ID '{}'", id));
        }
    }

    (void)newDatabase.setVersion(kLegacyBoundary + 1);
    return {};
}


} // namespace land::internal
