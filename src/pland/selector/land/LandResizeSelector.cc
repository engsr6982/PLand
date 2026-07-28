#include "LandResizeSelector.h"
#include "pland/PLand.h"
#include "pland/drawer/DrawHandleManager.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/selector/ISelector.h"

#include "mc/deps/core/math/Color.h"


namespace land {


LandResizeSelector::LandResizeSelector(Player& player, std::shared_ptr<Land> land)
: ISelector(player, land->getDimensionId(), land->is3D()),
  mLand(land) {
    mOldRangeDrawId = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(player)->draw(
        land->getAABB(),
        land->getDimensionId(),
        mce::Color::fromHexString(ConfigProvider::getDrawConfig().color.onResizeLandDrawOldRange)
    );
}

LandResizeSelector::~LandResizeSelector() {
    auto player = getPlayer();
    if (!player) {
        return;
    }

    if (mOldRangeDrawId) {
        PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(*player)->remove(mOldRangeDrawId);
    }
}

std::shared_ptr<Land> LandResizeSelector::getLand() const { return mLand.lock(); }


} // namespace land