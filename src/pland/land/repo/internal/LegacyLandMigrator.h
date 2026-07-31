#pragma once
#include "pland/infra/migrator/JsonMigrator.h"

namespace land {

struct LandContext;

namespace internal {

class LegacyLandMigrator : public infra::JsonMigrator<> {
public:
    explicit LegacyLandMigrator();

    static LegacyLandMigrator& getInstance();
};

} // namespace internal
} // namespace land
