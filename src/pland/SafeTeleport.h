#pragma once
#include "Global.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"


namespace land {


class SafeTeleport {

    class SafeTeleportImpl;

public:
    SafeTeleport()                               = default;
    SafeTeleport(const SafeTeleport&)            = delete;
    SafeTeleport& operator=(const SafeTeleport&) = delete;
    SafeTeleport(SafeTeleport&&)                 = delete;
    SafeTeleport& operator=(SafeTeleport&&)      = delete;

    LDAPI static SafeTeleport& getInstance();

    LDAPI void teleportTo(Player& player, Vec3 const& pos, int dimid);
};


} // namespace land