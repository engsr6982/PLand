#pragma once
#include <memory>

namespace mce {
class UUID;
}

namespace land {
class Land;
enum class LeaseState : uint8_t;
} // namespace land

namespace land::observer {

class ILandObserver {
public:
    virtual ~ILandObserver() = default;

    virtual void
    onOwnerChanged(std::shared_ptr<Land> const& land, mce::UUID const& oldOwner, mce::UUID const& newOwner) = 0;

    virtual void onMemberAdded(std::shared_ptr<Land> const& land, mce::UUID const& member) = 0;

    virtual void onMemberRemoved(std::shared_ptr<Land> const& land, mce::UUID const& member) = 0;

    virtual void onMembersCleared(std::shared_ptr<Land> const& land) = 0;

    virtual void onLeaseStateChanged(std::shared_ptr<Land> const& land, LeaseState oldState, LeaseState newState) = 0;

    virtual void onMarkDirty(std::shared_ptr<Land> const& land) = 0;
};

} // namespace land::observer