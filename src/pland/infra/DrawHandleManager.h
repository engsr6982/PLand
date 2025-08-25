#pragma once
#include "pland/Global.h"
#include <memory>
#include <unordered_map>

class Player;

namespace land {

class IDrawHandle;


class DrawHandleManager final {
    std::unordered_map<UUIDm, std::unique_ptr<IDrawHandle>> mDrawHandles;

    std::unique_ptr<IDrawHandle> createHandle() const;

public:
    LD_DISALLOW_COPY_AND_MOVE(DrawHandleManager);
    explicit DrawHandleManager();
    ~DrawHandleManager();

public:
    LDNDAPI IDrawHandle* getOrCreateHandle(Player& player);

    LDAPI void removeHandle(Player& player);

    LDAPI void removeAllHandle();
};


} // namespace land