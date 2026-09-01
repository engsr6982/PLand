#pragma once
#include "pland/infra/migrator/JsonMigrator.h"

#include <nlohmann/json_fwd.hpp>

namespace land {
namespace internal {

class ConfigMigrator : public infra::JsonMigrator<nlohmann::ordered_json> {
public:
    ConfigMigrator();

    static ConfigMigrator& getInstance();
};

} // namespace internal
} // namespace land
