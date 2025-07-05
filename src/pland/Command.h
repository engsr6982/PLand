#pragma once
#include "Global.h"

namespace land {

class LandCommand {
public:
    LandCommand()                              = delete;
    LandCommand(const LandCommand&)            = delete;
    LandCommand& operator=(const LandCommand&) = delete;
    ~LandCommand()                             = delete;

    LDAPI static bool setup();
};


} // namespace land