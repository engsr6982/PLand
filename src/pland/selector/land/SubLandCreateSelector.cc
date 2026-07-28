#include "SubLandCreateSelector.h"

#include "pland/PLand.h"
#include "pland/drawer/DrawHandleManager.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/selector/ISelector.h"

#include "mc/deps/core/math/Color.h"

namespace land {


SubLandCreateSelector::SubLandCreateSelector(Player& player, std::shared_ptr<Land> parent)
: ISelector(player, parent->getDimensionId(), true) {
    mParentLand        = parent;
    mParentRangeDrawId = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(player)->draw(
        parent->getAABB(),
        parent->getDimensionId(),
        mce::Color::fromHexString(ConfigProvider::getDrawConfig().color.onCreateSubLandDrawParentLand)
    );
}

SubLandCreateSelector::~SubLandCreateSelector() {
    auto player = getPlayer();
    if (!player) {
        return;
    }

    if (mParentRangeDrawId) {
        PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(*player)->remove(mParentRangeDrawId);
    }
}

std::shared_ptr<Land> SubLandCreateSelector::getParentLand() const { return mParentLand.lock(); }

std::shared_ptr<Land> SubLandCreateSelector::newSubLand() const {
    if (!isPointABSet()) {
        return nullptr;
    }

    auto parent = getParentLand();
    if (!parent) {
        return nullptr;
    }

    auto land = Land::make(
        *newLandAABB(),
        parent->getDimensionId(), // 子领地必须和父领地在一个维度
        true,                     // 子领地必须是3D
        parent->getOwner()        // 子领地属于父领地，所以父领地的拥有者也是子领地的拥有者
    );
    land->markDirty();
    return land;
}

} // namespace land
