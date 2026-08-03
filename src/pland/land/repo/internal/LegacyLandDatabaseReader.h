#pragma once
#include "pland/Global.h"


#include <ll/api/data/KeyValueDB.h>

namespace land {
namespace internal {

class LegacyLandDatabaseReader final {
    std::unique_ptr<ll::data::KeyValueDB> mDb;

public:
    inline static constexpr std::string_view kDatabaseDir            = "db";
    inline static constexpr std::string_view kDbVersionKey           = "__version__";
    inline static constexpr std::string_view kDbOperatorDataKey      = "operators";
    inline static constexpr std::string_view kDbPlayerSettingDataKey = "player_settings";
    inline static constexpr std::string_view kDbTemplatePermKey      = "template_perm";

    static std::filesystem::path getDatabasePath(std::filesystem::path const& dataDirectory);

    static bool isLandData(std::string_view key);

public:
    explicit LegacyLandDatabaseReader(std::unique_ptr<ll::data::KeyValueDB> db);

    [[nodiscard]] int32_t getVersion() const;

    [[nodiscard]] std::optional<std::string> getAdmins() const;

    [[nodiscard]] std::optional<std::string> getPlayerSettings() const;

    [[nodiscard]] std::optional<std::string> getTemplatePerm() const;

    [[nodiscard]] ll::coro::Generator<std::pair<LandID, std::string_view>> iterLands() const;
};

} // namespace internal
} // namespace land
