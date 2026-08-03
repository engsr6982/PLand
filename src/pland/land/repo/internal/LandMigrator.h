#pragma once
#include "pland/infra/migrator/JsonMigrator.h"

namespace land::internal {

class LandMigrator : public infra::JsonMigrator<> {
public:
    explicit LandMigrator();

    static LandMigrator& getInstance();
};

} // namespace land::internal
