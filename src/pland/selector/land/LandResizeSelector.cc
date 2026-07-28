#include "LandResizeSelector.h"
#include "pland/PLand.h"
#include "pland/drawer/DrawHandleManager.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/selector/ABSelector.h"

#include "mc/deps/core/math/Color.h"


namespace land {

struct LandResizeSelector::Impl {
    std::weak_ptr<Land> mLand;           // 领地
    drawer::GeoId       mOldRangeDrawId; // 旧领地范围
};

LandResizeSelector::LandResizeSelector(Player& player, std::shared_ptr<Land> land)
: ABSelector(player, land->getDimensionId(), land->is3D()),
  impl(std::make_unique<Impl>(land)) {
    impl->mOldRangeDrawId = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(player)->draw(
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

    if (impl->mOldRangeDrawId) {
        PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(*player)->remove(impl->mOldRangeDrawId);
    }
}

std::shared_ptr<Land> LandResizeSelector::getLand() const { return impl->mLand.lock(); }


} // namespace land