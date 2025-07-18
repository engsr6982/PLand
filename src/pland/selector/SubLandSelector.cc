#include "SubLandSelector.h"
#include "mc/deps/core/math/Color.h"
#include "pland/PLand.h"
#include "pland/infra/DrawHandleManager.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/land/Land.h"
#include "pland/selector/ISelector.h"


namespace land {


SubLandSelector::SubLandSelector(Player& player, SharedLand parent)
: ISelector(player, parent->getDimensionId(), true) {
    mParentLand        = parent;
    mParentRangeDrawId = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(player)->draw(
        parent->getAABB(),
        parent->getDimensionId(),
        mce::Color::RED()
    );
}

SubLandSelector::~SubLandSelector() {
    auto player = getPlayer();
    if (!player) {
        return;
    }

    if (mParentRangeDrawId) {
        PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(*player)->remove(mParentRangeDrawId);
    }
}

SharedLand SubLandSelector::getParentLand() const { return mParentLand.lock(); }

SharedLand SubLandSelector::newSubLand() const {
    if (!isPointABSet()) {
        return nullptr;
    }

    auto parent = getParentLand();
    if (!parent) {
        return nullptr;
    }

    return Land::make(
        *newLandAABB(),
        parent->getDimensionId(), // 子领地必须和父领地在一个维度
        true,                     // 子领地必须是3D
        parent->getOwner()        // 子领地属于父领地，所以父领地的拥有者也是子领地的拥有者
    );
}

} // namespace land
