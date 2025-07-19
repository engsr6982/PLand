#pragma once
#include "pland/selector/ISelector.h"


namespace land {

class Land;

class DefaultSelector final : public ISelector {
public:
    LDAPI explicit DefaultSelector(Player& player, bool is3D);

    LDNDAPI std::shared_ptr<Land> newLand() const;
};


} // namespace land