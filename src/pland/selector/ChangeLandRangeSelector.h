#pragma once
#include "pland/selector/ISelector.h"


namespace land {

struct GeoId;
class Land;

class ChangeLandRangeSelector final : public ISelector {
    std::weak_ptr<Land>    mLand;           // 领地
    std::unique_ptr<GeoId> mOldRangeDrawId; // 旧领地范围

public:
    LDAPI explicit ChangeLandRangeSelector(Player& player, std::shared_ptr<Land> land);
    LDAPI ~ChangeLandRangeSelector() override;

    LDNDAPI std::shared_ptr<Land> getLand() const;
};


} // namespace land