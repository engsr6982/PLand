#include "DirtyCounter.h"

namespace land {

DirtyCounter::DirtyCounter() = default;

bool DirtyCounter::isDirty() const { return mCounter > 0; }

int DirtyCounter::getCounter() const { return mCounter; }

void DirtyCounter::increment() { mCounter.fetch_add(1, std::memory_order_relaxed); }

void DirtyCounter::decrement() {
    if (mCounter > 0) mCounter.fetch_sub(1, std::memory_order_relaxed);
}

void DirtyCounter::reset() { mCounter.store(0, std::memory_order_relaxed); }


} // namespace land