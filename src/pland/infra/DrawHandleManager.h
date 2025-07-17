#pragma once
#include "pland/Global.h"
#include <memory>
#include <unordered_map>

class Player;

namespace land {

class IDrawHandle;


class DrawHandleManager final {
public:
    LD_DISALLOW_COPY_AND_MOVE(DrawHandleManager);

private:
    explicit DrawHandleManager();

    std::unordered_map<UUIDm, std::unique_ptr<IDrawHandle>> mDrawHandles;

public:
    LDNDAPI static DrawHandleManager& getInstance();

    LDNDAPI IDrawHandle* getOrCreateHandle(Player& player);

    LDAPI void removeHandle(Player& player);

    LDAPI void removeAllHandle();
};


} // namespace land