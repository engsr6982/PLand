#include "LandTemplatePermTable.h"

namespace land {


LandTemplatePermTable::LandTemplatePermTable(LandPermTable permTable) : mTemplatePermTable(permTable) {}

LandPermTable const& LandTemplatePermTable::get() const { return mTemplatePermTable; }

void LandTemplatePermTable::set(LandPermTable const& permTable) {
    mTemplatePermTable = permTable;
    mDirtyCounter.increment();
}


} // namespace land