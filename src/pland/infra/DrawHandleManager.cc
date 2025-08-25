#include "pland/infra/DrawHandleManager.h"
#include "mc/world/actor/player/Player.h"
#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/infra/DrawHandleType.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/infra/draw/impl/BSCIDrawHandle.h"
#include "pland/infra/draw/impl/DebugShapeHandle.h"
#include "pland/infra/draw/impl/DefaultDrawHandle.h"
#include <memory>


namespace land {


DrawHandleManager::DrawHandleManager() {
    auto& logger = PLand::getInstance().getSelf().getLogger();
    if (Config::cfg.land.drawHandleBackend == DrawHandleBackend::BedrockServerClientInterfaceMod
        && !BsciDrawHandle::isBsciModuleLoaded()) {
        logger.warn(
            "[DrawHandleManager] The BedrockServerClientInterface module is not loaded, and the plugin uses "
            "the built-in particle system!"
        );
        logger.warn("[DrawHandleManager] BedrockServerClientInterface 模块未加载，插件将使用内置粒子系统!");
        Config::cfg.land.drawHandleBackend = DrawHandleBackend::DefaultParticle;
        Config::trySave();
    }
}

DrawHandleManager::~DrawHandleManager() = default;

std::unique_ptr<IDrawHandle> DrawHandleManager::createHandle() const {
    switch (Config::cfg.land.drawHandleBackend) {
    case DrawHandleBackend::DefaultParticle:
        return std::make_unique<DefaultDrawHandle>();
    case DrawHandleBackend::BedrockServerClientInterfaceMod:
        return std::make_unique<BsciDrawHandle>();
    case DrawHandleBackend::MinecraftDebugShape:
        return std::make_unique<DebugShapeHandle>();
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
