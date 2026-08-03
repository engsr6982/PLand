#pragma once
#include <ll/api/Expected.h>

#include <memory>

namespace ll::data {
class KeyValueDB;
}

namespace land ::internal {
class LegacyLandDatabaseReader;
class LandDatabase;

class LegacyLandDatabaseUpgrader final {
public:
    // Boundary version separating legacy JSON data (<= 31) from modern BEVE data (>= 32)
    inline static constexpr int32_t kLegacyBoundary = 31;

    static bool isLegacyDatabase(int32_t databaseVersion);

    [[nodiscard]] static ll::Expected<>
    upgrade(std::unique_ptr<ll::data::KeyValueDB> legacyDatabase, LandDatabase& newDatabase);
};

} // namespace land::internal
