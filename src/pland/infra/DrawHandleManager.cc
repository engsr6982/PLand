#include "pland/infra/DrawHandleManager.h"
#include "mc/world/actor/player/Player.h"
#include "pland/PLand.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/infra/draw/impl/BSCIDrawHandle.h"
#include "pland/infra/draw/impl/DefaultDrawHandle.h"


namespace land {


DrawHandleManager::DrawHandleManager() : mBsciAvailable(BsciDrawHandle::isBsciModuleLoaded()) {
    auto& logger = PLand::getInstance().getSelf().getLogger();
    logger.trace("[DrawHandleManager] Check the dependency status");

    if (mBsciAvailable) {
        logger.trace("[DrawHandleManager] BedrockServerClientInterface module is loaded");
    } else {
        logger.warn("[DrawHandleManager] The BedrockServerClientInterface module is not loaded, and the plugin uses "
                    "the built-in particle system!");
        logger.warn("[DrawHandleManager] BedrockServerClientInterface 模块未加载，插件将使用内置粒子系统!");
    }
}

DrawHandleManager::~DrawHandleManager() = default;

std::unique_ptr<IDrawHandle> DrawHandleManager::createHandle() const {
    if (mBsciAvailable) {
        return std::make_unique<BsciDrawHandle>();
    } else {
        return std::make_unique<DefaultDrawHandle>();
    }
}

IDrawHandle* DrawHandleManager::getOrCreateHandle(Player& player) {
    auto iter = mDrawHandles.find(player.getUuid());
    if (iter == mDrawHandles.end()) {
        auto handle = createHandle();
        iter        = mDrawHandles.emplace(player.getUuid(), std::move(handle)).first;
    }
    return iter->second.get();
}

void DrawHandleManager::removeHandle(Player& player) { mDrawHandles.erase(player.getUuid()); }

void DrawHandleManager::removeAllHandle() { mDrawHandles.clear(); }


} // namespace land
