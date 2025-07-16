#pragma once
#include "pland/infra/DrawHandleManager.h"
#include "pland/land/Land.h"
#include "pland/selector/ISelector.h"


namespace land {


class SubLandSelector final : public ISelector {
    WeakLand                 mParentLand;
    DrawHandle::UniqueDrawId mParentRangeDrawId;

public:
    LDAPI explicit SubLandSelector(Player& player, SharedLand parent);
    LDAPI ~SubLandSelector() override;

    LDNDAPI SharedLand getParentLand() const;

    LDNDAPI SharedLand newSubLand() const;
};


} // namespace land