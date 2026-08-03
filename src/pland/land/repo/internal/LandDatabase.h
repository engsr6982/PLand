#pragma once
#include "pland/Global.h"
#include "pland/reflect/SerializeType.h"
#include "pland/utils/JsonUtil.h"

#include <ll/api/Expected.h>
#include <ll/api/coro/Generator.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ll::data {
class KeyValueDB;
}

namespace land ::internal {

class LandDatabase {
    std::unique_ptr<ll::data::KeyValueDB> mDb;

public:
    inline static constexpr std::string_view kDatabaseDirName      = "database_v2";
    inline static constexpr std::string_view kDatabaseVersionKey   = "meta:version";
    inline static constexpr std::string_view kAdminsKey            = "data:admins";
    inline static constexpr std::string_view kPlayerSettingsKey    = "data:player_settings";
    inline static constexpr std::string_view kTemplatePermTableKey = "data:template_perm_table";
    inline static constexpr std::string_view kLandContextPrefix    = "data:land_ctx:";

    static std::filesystem::path getDatabasePath(std::filesystem::path dataDir);

    static std::string buildLandContextKey(LandID id);

    static bool isLandContext(std::string_view key);

public:
    explicit LandDatabase(std::filesystem::path const& path);
    ~LandDatabase();

    ll::Expected<int64_t> snapshotTo(ll::data::KeyValueDB& other);

    [[nodiscard]] std::optional<int32_t> getVersion() const;
    [[nodiscard]] bool                   setVersion(int32_t version);

    [[nodiscard]] bool empty() const;

    [[nodiscard]] bool has(std::string_view key) const;

    [[nodiscard]] bool del(std::string_view key);

    [[nodiscard]] bool set(std::string_view key, std::string_view val);

    [[nodiscard]] std::optional<std::string> get(std::string_view key) const;

    struct DataView {
        std::string_view mBuffer;

        [[nodiscard]] inline std::string_view as_str() const { return mBuffer; }

        template <typename J = nlohmann::json>
        [[nodiscard]] ll::Expected<J> as_json() const {
            auto striped = stripChecksum(mBuffer);
            if (!striped) {
                return ll::makeStringError("Data integrity check failed: Checksum mismatch or invalid header length");
            }
            return J::from_cbor(*striped);
        }
    };
    [[nodiscard]] ll::coro::Generator<DataView> iter(std::string_view prefix) const;

    template <typename T>
    [[nodiscard]] bool save(std::string_view key, T const& data) {
        auto j      = json_util::struct_to_json<T, nlohmann::json>(data);
        auto buffer = nlohmann::json::to_cbor(j);
        if (buffer.empty()) [[unlikely]] {
            return false;
        }
        auto strBuf = std::string{buffer.begin(), buffer.end()};
        return saveBinary(key, std::move(strBuf));
    }

    template <typename T>
    [[nodiscard]] ll::Expected<> readTo(std::string_view key, T& object) const {
        auto binaryData = readBinary(key);
        if (!binaryData) {
            return ll::makeStringError(fmt::format("Failed to read or verify binary payload for key: '{}'", key));
        }
        auto j = nlohmann::json::from_cbor(*binaryData);
        return json_util::json_to_struct<T, nlohmann::json>(j, object);
    }

private:
    bool saveBinary(std::string_view key, std::string buffer);

    std::optional<std::string> readBinary(std::string_view key) const;

    friend DataView;
    friend class LegacyLandDatabaseUpgrader;
    static void                                          appendChecksum(std::string& buffer);
    [[nodiscard]] static std::optional<std::string_view> stripChecksum(std::string_view raw) noexcept;
};

} // namespace land::internal
