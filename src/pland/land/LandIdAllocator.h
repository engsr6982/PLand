#pragma once
#include "pland/Global.h"

namespace land {


class LandIdAllocator {
    std::atomic<LandID> mCurrentId;

public:
    LDAPI explicit LandIdAllocator(LandID currentId = 0);

    LDAPI LandID nextId();
};


} // namespace land