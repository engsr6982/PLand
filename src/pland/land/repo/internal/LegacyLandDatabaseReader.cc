#include "LegacyLandDatabaseReader.h"

namespace land {
namespace internal {

std::filesystem::path LegacyLandDatabaseReader::getDatabasePath(std::filesystem::path const& dataDirectory) {
    return dataDirectory / kDatabaseDir;
}

bool LegacyLandDatabaseReader::isLandData(std::string_view key) {
    return key != kDbVersionKey && key != kDbOperatorDataKey && key != kDbPlayerSettingDataKey
        && key != kDbTemplatePermKey;
}

LegacyLandDatabaseReader::LegacyLandDatabaseReader(std::unique_ptr<ll::data::KeyValueDB> db) : mDb(std::move(db)) {}

int32_t LegacyLandDatabaseReader::getVersion() const { return std::stoi(mDb->get(kDbVersionKey).value_or("-1")); }

std::optional<std::string> LegacyLandDatabaseReader::getAdmins() const { return mDb->get(kDbOperatorDataKey); }

std::optional<std::string> LegacyLandDatabaseReader::getPlayerSettings() const {
    return mDb->get(kDbPlayerSettingDataKey);
}

std::optional<std::string> LegacyLandDatabaseReader::getTemplatePerm() const { return mDb->get(kDbTemplatePermKey); }

ll::coro::Generator<std::pair<LandID, std::string_view>> LegacyLandDatabaseReader::iterLands() const {
    auto iter = mDb->iter();
    for (auto [k, v] : iter) {
        if (isLandData(k)) {
            auto id = std::stoll(std::string(k));
            co_yield {id, v};
        }
    }
}


} // namespace internal
} // namespace land