#include "LandTemplatePermTable.h"

namespace land {


LandTemplatePermTable::LandTemplatePermTable(LandPermTable permTable) : mTemplatePermTable(permTable) {}

LandPermTable LandTemplatePermTable::get() const {
    std::shared_lock lock(mMutex);
    return mTemplatePermTable;
}

void LandTemplatePermTable::set(LandPermTable const& permTable) {
    std::unique_lock lock(mMutex);
    mTemplatePermTable = permTable;
    mDirty.store(true, std::memory_order_release);
}
bool LandTemplatePermTable::isDirty() const { return mDirty.load(std::memory_order_acquire); }
void LandTemplatePermTable::markDirty() { mDirty.store(true, std::memory_order_release); }
void LandTemplatePermTable::resetDirty() { mDirty.store(false, std::memory_order_release); }


} // namespace land
