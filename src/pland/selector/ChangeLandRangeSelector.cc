#include "ChangeLandRangeSelector.h"
#include "mc/deps/core/math/Color.h"
#include "pland/infra/DrawHandleManager.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/land/Land.h"
#include "pland/selector/ISelector.h"


namespace land {


ChangeLandRangeSelector::ChangeLandRangeSelector(Player& player, SharedLand land)
: ISelector(player, land->getDimensionId(), land->is3D()) {
    mOldRangeDrawId = DrawHandleManager::getInstance().getOrCreateHandle(player)->draw(
        land->getAABB(),
        land->getDimensionId(),
        mce::Color::ORANGE()
    );
}

ChangeLandRangeSelector::~ChangeLandRangeSelector() {
    auto player = getPlayer();
    if (player) {
        return;
    }

    if (mOldRangeDrawId) {
        DrawHandleManager::getInstance().getOrCreateHandle(*player)->remove(mOldRangeDrawId);
    }
}

SharedLand ChangeLandRangeSelector::getLand() const { return mLand.lock(); }


} // namespace land