#include "LandIdAllocator.h"


namespace land {


LandIdAllocator::LandIdAllocator(LandID currentId) { mCurrentId.store(currentId); }

LandID LandIdAllocator::nextId() { return mCurrentId.fetch_add(1); }


} // namespace land