#pragma once
#include "pland/infra/migrator/JsonMigrator.h"

namespace land {
namespace internal {

class ConfigMigrator : public infra::BasicJsonMigrator<nlohmann::ordered_json> {
public:
    ConfigMigrator();

    static ConfigMigrator& getInstance();
};

} // namespace internal
} // namespace land
