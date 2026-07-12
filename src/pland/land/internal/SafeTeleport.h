#pragma once
#include "mc/world/level/dimension/DimensionType.h"
#include "pland/Global.h"

class BlockPos;
class Player;

namespace land::internal {

class SafeTeleport {
public:
    explicit SafeTeleport();
    ~SafeTeleport();

    void launchTask(Player& player, BlockPos const& targetPos, DimensionType dimension);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace land::internal
