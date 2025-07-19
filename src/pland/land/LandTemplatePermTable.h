#pragma once
#include "LandContext.h"
#include "pland/infra/DirtyCounter.h"


namespace land {

class LandRegistry;

class LandTemplatePermTable {
public:
    LDAPI explicit LandTemplatePermTable(LandPermTable permTable);

    LandPermTable const& get() const;

    void set(LandPermTable const& permTable);

private:
    DirtyCounter  mDirtyCounter;
    LandPermTable mTemplatePermTable;

    friend class LandRegistry;
};


} // namespace land