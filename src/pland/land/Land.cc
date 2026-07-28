#include "pland/land/Land.h"

#include "mc/platform/UUID.h"

#include "observer/ILandObserver.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/enums/LandRole.h"
#include "pland/land/Config.h"
#include "pland/utils/JsonUtil.h"
#include "repo/LandContext.h"


#include <unordered_set>
#include <vector>


namespace land {

struct Land::Impl {
    LandContext  mContext;
    DirtyCounter mDirtyCounter;

    observer::ILandObserver* mObserver{nullptr};

    // cache
    mutable std::optional<mce::UUID>      mCacheOwner;
    mutable std::unordered_set<mce::UUID> mCacheMembers;
    mutable std::optional<int>            mCacheNestedLevel;

    void initCache() {
        mCacheOwner = std::nullopt;
        mCacheMembers.clear();

        if (!mContext.mOwnerDataIsXUID) {
            mCacheOwner = mce::UUID::fromString(mContext.mLandOwner);
        }
        mCacheMembers.reserve(mContext.mLandMembers.size());
        for (auto const& member : mContext.mLandMembers) {
            mCacheMembers.emplace(mce::UUID::fromString(member));
        }
    }
};

Land::Land() : impl(std::make_unique<Impl>()) {}
Land::Land(LandContext ctx) : Land{} { impl->mContext = std::move(ctx); }
Land::Land(LandAABB const& pos, LandDimid dimid, bool is3D, mce::UUID const& owner, LandPermTable ptable) : Land{} {
    impl->mContext.mPos           = pos;
    impl->mContext.mLandDimid     = dimid;
    impl->mContext.mIs3DLand      = is3D;
    impl->mContext.mLandOwner     = owner.asString();
    impl->mContext.mLandPermTable = ptable;

    impl->initCache();
}
Land::~Land() = default;

LandAABB const& Land::getAABB() const { return impl->mContext.mPos; }

LandPos const& Land::getTeleportPos() const { return impl->mContext.mTeleportPos; }
bool           Land::setTeleportPos(LandPos const& pos) {
    if (getAABB().hasPos(pos.as<>())) {
        impl->mContext.mTeleportPos = pos;
        impl->mDirtyCounter.increment();
        return true;
    }
    return false;
}

LandID    Land::getId() const { return impl->mContext.mLandID; }
LandDimid Land::getDimensionId() const { return impl->mContext.mLandDimid; }

LandPermTable const& Land::getPermTable() const { return impl->mContext.mLandPermTable; }
void                 Land::setPermTable(LandPermTable permTable) {
    impl->mContext.mLandPermTable = permTable;
    impl->mDirtyCounter.increment();
}

mce::UUID const& Land::getOwner() const {
    if (!impl->mCacheOwner) {
        if (impl->mContext.mOwnerDataIsXUID) {
            return mce::UUID::EMPTY();
        }
        impl->mCacheOwner = mce::UUID(impl->mContext.mLandOwner);
    }
    return *impl->mCacheOwner;
}
void Land::setOwner(mce::UUID const& uuid) {
    auto old = impl->mCacheOwner.value_or(mce::UUID::EMPTY());
    if (uuid != old) {
        impl->mCacheOwner         = uuid;
        impl->mContext.mLandOwner = uuid.asString();
        markDirty();
        if (auto observer = tryGetObserver()) {
            observer->onOwnerChanged(shared_from_this(), old, uuid);
        }
    }
}
std::string const& Land::getRawOwner() const { return impl->mContext.mLandOwner; }
bool               Land::isSystemOwned() const {
    assert(SYSTEM_ACCOUNT_UUID != mce::UUID::EMPTY());
    assert(SYSTEM_ACCOUNT_UUID.asString() == SYSTEM_ACCOUNT_UUID_STR);
    return impl->mCacheOwner == SYSTEM_ACCOUNT_UUID;
}

std::unordered_set<mce::UUID> const& Land::getMembers() const { return impl->mCacheMembers; }

bool Land::addLandMember(mce::UUID const& uuid) {
    if (isOwner(uuid) || isMember(uuid)) {
        return false;
    }
    impl->mCacheMembers.insert(uuid);
    impl->mContext.mLandMembers.emplace_back(uuid.asString());
    markDirty();
    if (auto observer = tryGetObserver()) {
        observer->onMemberAdded(shared_from_this(), uuid);
    }
    return true;
}
bool Land::removeLandMember(mce::UUID const& uuid) {
    if (!isMember(uuid)) {
        return false;
    }
    impl->mCacheMembers.erase(uuid);
    std::erase_if(impl->mContext.mLandMembers, [uuid = uuid.asString()](auto const& u) { return u == uuid; });
    markDirty();
    if (auto observer = tryGetObserver()) {
        observer->onMemberRemoved(shared_from_this(), uuid);
    }
    return true;
}
void Land::clearMembers() {
    impl->mCacheMembers.clear();
    impl->mContext.mLandMembers.clear();
    markDirty();
    if (auto observer = tryGetObserver()) {
        observer->onMembersCleared(shared_from_this());
    }
}

std::string const& Land::getName() const { return impl->mContext.mLandName; }
void               Land::setName(std::string const& name) {
    impl->mContext.mLandName = name;
    impl->mDirtyCounter.increment();
}

int  Land::getOriginalBuyPrice() const { return impl->mContext.mOriginalBuyPrice; }
void Land::setOriginalBuyPrice(int price) {
    impl->mContext.mOriginalBuyPrice = price;
    impl->mDirtyCounter.increment();
}

LandHoldType Land::getHoldType() const { return impl->mContext.mHoldType; }
LeaseState   Land::getLeaseState() const { return impl->mContext.mLeasing.mState; }
bool         Land::isBought() const { return impl->mContext.mHoldType == LandHoldType::Bought; }
bool         Land::isLeased() const { return impl->mContext.mHoldType == LandHoldType::Leased; }
bool         Land::isLeaseActive() const { return isLeased() && impl->mContext.mLeasing.mState == LeaseState::Active; }
bool         Land::isLeaseFrozen() const { return isLeased() && impl->mContext.mLeasing.mState == LeaseState::Frozen; }
bool   Land::isLeaseExpired() const { return isLeased() && impl->mContext.mLeasing.mState == LeaseState::Expired; }
time_t Land::getLeaseStartAt() const { return impl->mContext.mLeasing.mStartAt; }
time_t Land::getLeaseEndAt() const { return impl->mContext.mLeasing.mEndAt; }

void Land::setHoldType(LandHoldType type) {
    impl->mContext.mHoldType = type;
    impl->mDirtyCounter.increment();
}
void Land::setLeaseState(LeaseState state) {
    auto old = impl->mContext.mLeasing.mState;
    if (old != state) {
        impl->mContext.mLeasing.mState = state;
        impl->mDirtyCounter.increment();
        if (auto observer = tryGetObserver()) {
            observer->onLeaseStateChanged(shared_from_this(), old, state);
        }
    }
}
void Land::setLeaseStartAt(time_t ts) {
    impl->mContext.mLeasing.mStartAt = ts;
    impl->mDirtyCounter.increment();
}
void Land::setLeaseEndAt(time_t ts) {
    impl->mContext.mLeasing.mEndAt = ts;
    impl->mDirtyCounter.increment();
}

bool                Land::is3D() const { return impl->mContext.mIs3DLand; }
bool                Land::isOwner(mce::UUID const& uuid) const { return impl->mCacheOwner == uuid; }
bool                Land::isMember(mce::UUID const& uuid) const { return impl->mCacheMembers.contains(uuid); }
bool                Land::isConvertedLand() const { return impl->mContext.mIsConvertedLand; }
bool                Land::isOwnerDataIsXUID() const { return impl->mContext.mOwnerDataIsXUID; }
bool                Land::isDirty() const { return impl->mDirtyCounter.isDirty(); }
void                Land::markDirty() { impl->mDirtyCounter.increment(); }
void                Land::rollbackDirty() { impl->mDirtyCounter.decrement(); }
DirtyCounter&       Land::getDirtyCounter() { return impl->mDirtyCounter; }
DirtyCounter const& Land::getDirtyCounter() const { return impl->mDirtyCounter; }

LandType Land::getType() const {
    if (isOrdinaryLand()) [[likely]] {
        return LandType::Ordinary;
    }
    if (isParentLand()) {
        return LandType::Parent;
    }
    if (isMixLand()) {
        return LandType::Mix;
    }
    if (isSubLand()) {
        return LandType::Sub;
    }
    [[unlikely]] throw std::runtime_error("Unknown land type");
}
bool Land::hasParentLand() const { return impl->mContext.mParentLandID != INVALID_LAND_ID; }
bool Land::hasSubLand() const { return !impl->mContext.mSubLandIDs.empty(); }

bool Land::isOrdinaryLand() const { return !hasParentLand() && !hasSubLand(); } // 无父 & 无子
bool Land::isParentLand() const { return !hasParentLand() && hasSubLand(); }    // 无父 & 有子
bool Land::isMixLand() const { return hasParentLand() && hasSubLand(); }        // 有父 & 有子
bool Land::isSubLand() const { return hasParentLand() && !hasSubLand(); }       // 有父 & 无子

bool Land::canCreateSubLand() const {
    if (isLeased()) return false;
    auto const& conf        = ConfigProvider::getSubLandConfig();
    auto        nestedLevel = getNestedLevel();
    return nestedLevel < conf.maxNestedDepth && nestedLevel < GlobalSubLandMaxNestedLevel
        && static_cast<int>(impl->mContext.mSubLandIDs.size()) < conf.maxSubLandsPerLand;
}

LandID              Land::getParentLandID() const { return impl->mContext.mParentLandID; }
std::vector<LandID> Land::getSubLandIDs() const { return impl->mContext.mSubLandIDs; }

int Land::getNestedLevel() const { return impl->mCacheNestedLevel.value_or(0); }


bool Land::isCollision(BlockPos const& pos, int radius) const {
    BlockPos minPos(
        pos.x - radius,
        impl->mContext.mIs3DLand ? pos.y - radius : impl->mContext.mPos.min.y,
        pos.z - radius
    );
    BlockPos maxPos(
        pos.x + radius,
        impl->mContext.mIs3DLand ? pos.y + radius : impl->mContext.mPos.max.y,
        pos.z + radius
    );
    return isCollision(minPos, maxPos);
}

bool Land::isCollision(BlockPos const& pos1, BlockPos const& pos2) const {
    return LandAABB::isCollision(
        impl->mContext.mPos,
        LandAABB{
            LandPos{pos1.x, pos1.y, pos1.z},
            LandPos{pos2.x, pos2.y, pos2.z}
    }
    );
}


LandPermType Land::getPermType(mce::UUID const& uuid) const {
    if (isLeaseFrozen()) {
        return LandPermType::Actor;
    }
    if (isOwner(uuid)) return LandPermType::Owner;
    if (isMember(uuid)) return LandPermType::Member;
    return LandPermType::Actor;
}

void Land::migrateOwner(mce::UUID const& ownerUUID) {
    if (isConvertedLand() && isOwnerDataIsXUID()) {
        setOwner(ownerUUID);
        impl->mContext.mOwnerDataIsXUID = false;
        impl->mDirtyCounter.increment();
    }
}

void Land::load(nlohmann::json& json) {
    json_util::json2structWithVersionPatch(json, impl->mContext, true);
    impl->initCache();
}
nlohmann::json Land::toJson() const { return json_util::struct2json(impl->mContext); }

bool Land::operator==(Land const& other) const { return impl->mContext.mLandID == other.impl->mContext.mLandID; }


// friend LandHierarchyService API
LandContext& Land::_getContext() const { return impl->mContext; }
void         Land::_setCachedNestedLevel(int level) { impl->mCacheNestedLevel = level; }
void         Land::_setLandId(LandID id) { impl->mContext.mLandID = id; }
void         Land::_reinit(LandContext context, unsigned int dirtyDiff) {
    impl->mContext = std::move(context);
    impl->mDirtyCounter.reset(dirtyDiff);
    impl->initCache();
}
bool Land::_setAABB(LandAABB const& newRange) {
    if (!isOrdinaryLand()) {
        return false;
    }
    impl->mContext.mPos = newRange;
    markDirty();
    return true;
}

observer::ILandObserver* Land::tryGetObserver() const { return impl->mObserver; }
void                     Land::setObserver(observer::ILandObserver* observer) {
    assert(observer != nullptr);
    impl->mObserver = observer;
}

} // namespace land
