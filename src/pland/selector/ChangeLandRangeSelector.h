#pragma once
#include "pland/infra/DrawHandleManager.h"
#include "pland/land/Land.h"
#include "pland/selector/ISelector.h"


namespace land {


class ChangeLandRangeSelector final : public ISelector {
    WeakLand                 mLand;           // 领地
    DrawHandle::UniqueDrawId mOldRangeDrawId; // 旧领地范围

public:
    LDAPI explicit ChangeLandRangeSelector(Player& player, SharedLand land);
    LDAPI ~ChangeLandRangeSelector() override;

    LDNDAPI SharedLand getLand() const;
};


} // namespace land