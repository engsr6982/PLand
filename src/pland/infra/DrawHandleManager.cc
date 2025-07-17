#include "pland/infra/DrawHandleManager.h"
#include "mc/world/actor/player/Player.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/infra/draw/impl/BSCIDrawHandle.h"


namespace land {


DrawHandleManager::DrawHandleManager() = default;

DrawHandleManager& DrawHandleManager::getInstance() {
    static DrawHandleManager instance;
    return instance;
}

IDrawHandle* DrawHandleManager::getOrCreateHandle(Player& player) {
    auto iter = mDrawHandles.find(player.getUuid());
    if (iter == mDrawHandles.end()) {
        auto bsciHandle = std::make_unique<BsciDrawHandle>();
        iter            = mDrawHandles.emplace(player.getUuid(), std::move(bsciHandle)).first;
    }
    return iter->second.get();
}

void DrawHandleManager::removeHandle(Player& player) { mDrawHandles.erase(player.getUuid()); }

void DrawHandleManager::removeAllHandle() { mDrawHandles.clear(); }


} // namespace land
