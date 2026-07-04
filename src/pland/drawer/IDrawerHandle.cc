#include "IDrawerHandle.h"

#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/actor/player/Player.h"

namespace land::drawer {

IDrawerHandle::IDrawerHandle()  = default;
IDrawerHandle::~IDrawerHandle() = default;

void IDrawerHandle::setTargetPlayer(Player& player) { mTargetPlayer = player.getEntityContext().getWeakRef(); }

optional_ref<Player> IDrawerHandle::getTargetPlayer() const {
    auto mob = mTargetPlayer.tryUnwrap<Mob>();
    if (!mob || mob->getEntityTypeId() != ActorType::Player) {
        return nullptr;
    }
    return static_cast<Player*>(mob.as_ptr());
}

} // namespace land::drawer
