#include "pland/land/Land.h"
#include "pland/Global.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"
#include "pland/utils/JSON.h"
#include <stack>
#include <vector>

namespace land {


Land::Land() = default;
Land::Land(LandContext ctx) : mContext(std::move(ctx)) {}
Land::Land(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner) {
    mContext.mPos       = pos;
    mContext.mLandDimid = dimid;
    mContext.mIs3DLand  = is3D;
    mContext.mLandOwner = owner;
}

LandAABB const& Land::getAABB() const { return mContext.mPos; }
void            Land::setAABB(LandAABB const& pos) {
    if (!isOrdinaryLand()) {
        return;
    }
    mContext.mPos = pos;
    mDirtyCounter.increment();
}

LandPos const& Land::getTeleportPos() const { return mContext.mTeleportPos; }
void           Land::setTeleportPos(LandPos const& pos) {
    mContext.mTeleportPos = pos;
    mDirtyCounter.increment();
}

LandID    Land::getId() const { return mContext.mLandID; }
LandDimid Land::getDimensionId() const { return mContext.mLandDimid; }

LandPermTable const& Land::getPermTable() const { return mContext.mLandPermTable; }
void                 Land::setPermTable(LandPermTable permTable) {
    mContext.mLandPermTable = std::move(permTable);
    mDirtyCounter.increment();
}

UUIDs const& Land::getOwner() const { return mContext.mLandOwner; }
void         Land::setOwner(UUIDs const& uuid) {
    mContext.mLandOwner = uuid;
    mDirtyCounter.increment();
}

std::vector<UUIDs> const& Land::getMembers() const { return mContext.mLandMembers; }
void                      Land::addLandMember(UUIDs const& uuid) {
    mContext.mLandMembers.push_back(uuid);
    mDirtyCounter.increment();
}
void Land::removeLandMember(UUIDs const& uuid) {
    std::erase_if(mContext.mLandMembers, [uuid](UUIDs const& u) { return u == uuid; });
    mDirtyCounter.increment();
}

std::string const& Land::getName() const { return mContext.mLandName; }
void               Land::setName(std::string const& name) {
    mContext.mLandName = name;
    mDirtyCounter.increment();
}

std::string const& Land::getDescribe() const { return mContext.mLandDescribe; }
void               Land::setDescribe(std::string const& describe) {
    mContext.mLandDescribe = std::string(describe);
    mDirtyCounter.increment();
}

int  Land::getOriginalBuyPrice() const { return mContext.mOriginalBuyPrice; }
void Land::setOriginalBuyPrice(int price) {
    mContext.mOriginalBuyPrice = price;
    mDirtyCounter.increment();
}

bool Land::is3D() const { return mContext.mIs3DLand; }
bool Land::isOwner(UUIDs const& uuid) const { return mContext.mLandOwner == uuid; }
bool Land::isMember(UUIDs const& uuid) const {
    return std::ranges::find(mContext.mLandMembers, uuid) != mContext.mLandMembers.end();
}
bool Land::isConvertedLand() const { return mContext.mIsConvertedLand; }
bool Land::isOwnerDataIsXUID() const { return mContext.mOwnerDataIsXUID; }
bool Land::isDirty() const { return mDirtyCounter.isDirty(); }

Land::Type Land::getType() const {
    if (isOrdinaryLand()) [[likely]] {
        return Type::Normal;
    } else if (isParentLand()) {
        return Type::Parent;
    } else if (isMixLand()) {
        return Type::Mix;
    } else if (isSubLand()) {
        return Type::Sub;
    }
    throw std::runtime_error("Unknown land type"); [[unlikely]];
}
bool Land::hasParentLand() const { return this->mContext.mParentLandID != static_cast<LandID>(-1); }
bool Land::hasSubLand() const { return !this->mContext.mSubLandIDs.empty(); }
bool Land::isSubLand() const {
    return this->mContext.mParentLandID != static_cast<LandID>(-1) && this->mContext.mSubLandIDs.empty();
}
bool Land::isParentLand() const {
    return this->mContext.mParentLandID == static_cast<LandID>(-1) && !this->mContext.mSubLandIDs.empty();
}
bool Land::isMixLand() const {
    return this->mContext.mParentLandID != static_cast<LandID>(-1) && !this->mContext.mSubLandIDs.empty();
}
bool Land::isOrdinaryLand() const {
    return this->mContext.mParentLandID == static_cast<LandID>(-1) && this->mContext.mSubLandIDs.empty();
}
bool Land::canCreateSubLand() const {
    auto nestedLevel = getNestedLevel();
    return nestedLevel < Config::cfg.land.subLand.maxNested && nestedLevel < GlobalSubLandMaxNestedLevel
        && static_cast<int>(this->mContext.mSubLandIDs.size()) < Config::cfg.land.subLand.maxSubLand;
}

SharedLand Land::getParentLand() const {
    if (isParentLand() || !hasParentLand()) {
        return nullptr;
    }
    return LandRegistry::getInstance().getLand(this->mContext.mParentLandID);
}

std::vector<SharedLand> Land::getSubLands() const {
    if (!hasSubLand()) {
        return {};
    }
    return LandRegistry::getInstance().getLands(this->mContext.mSubLandIDs);
}
int Land::getNestedLevel() const {
    if (!hasParentLand()) {
        return 0;
    }

    std::stack<SharedLand> stack;
    stack.push(getParentLand());
    int level = 0;
    while (!stack.empty()) {
        auto land = stack.top();
        stack.pop();
        level++;
        if (land->hasParentLand()) {
            stack.push(land->getParentLand());
        }
    }
    return level;
}
SharedLand Land::getRootLand() const {
    if (!hasParentLand()) {
        return LandRegistry::getInstance().getLand(this->mContext.mLandID); // 如果是父领地，直接返回自己
    }

    SharedLand root = getParentLand();
    while (root->hasParentLand()) {
        root = root->getParentLand();
    }

    return root;
}

bool Land::isCollision(BlockPos const& pos, int radius) const {
    BlockPos minPos(pos.x - radius, mContext.mIs3DLand ? pos.y - radius : mContext.mPos.min.y, pos.z - radius);
    BlockPos maxPos(pos.x + radius, mContext.mIs3DLand ? pos.y + radius : mContext.mPos.max.y, pos.z + radius);
    return isCollision(minPos, maxPos);
}

bool Land::isCollision(BlockPos const& pos1, BlockPos const& pos2) const {
    return LandAABB::isCollision(
        mContext.mPos,
        LandAABB{
            LandPos{pos1.x, pos1.y, pos1.z},
            LandPos{pos2.x, pos2.y, pos2.z}
    }
    );
}


LandPermType Land::getPermType(UUIDs const& uuid) const {
    if (uuid.empty()) return LandPermType::Guest; // empty uuid is guest
    if (isOwner(uuid)) return LandPermType::Owner;
    if (isMember(uuid)) return LandPermType::Member;
    return LandPermType::Guest;
}

void Land::updateXUIDToUUID(UUIDs const& ownerUUID) {
    if (isConvertedLand() && isOwnerDataIsXUID()) {
        mContext.mLandOwner       = ownerUUID;
        mContext.mOwnerDataIsXUID = false;
        mDirtyCounter.increment();
    }
}

void           Land::load(nlohmann::json& json) { JSON::jsonToStruct(json, mContext); }
nlohmann::json Land::dump() const { return JSON::structTojson(mContext); }
void           Land::save(bool force) {
    if (isDirty() || force) {
        if (LandRegistry::getInstance().save(*this)) {
            mDirtyCounter.reset();
        }
    }
}


bool Land::operator==(SharedLand const& other) const { return mContext.mLandID == other->mContext.mLandID; }


// static
SharedLand Land::make() { return std::make_shared<Land>(); }
SharedLand Land::make(LandContext ctx) { return std::make_shared<Land>(std::move(ctx)); }
SharedLand Land::make(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner) {
    return std::make_shared<Land>(pos, dimid, is3D, owner);
}

llong Land::calculatePriceRecursively(SharedLand const& land, RecursionCalculationPriceHandle const& handle) {
    std::stack<SharedLand> stack;
    stack.push(land);

    llong price = 0;

    while (!stack.empty()) {
        SharedLand current = stack.top();
        stack.pop();

        if (handle) {
            if (!handle(current, price)) break; // if handle return false, break
        } else {
            price += current->mContext.mOriginalBuyPrice;
        }

        if (current->hasSubLand()) {
            for (auto& subLand : current->getSubLands()) {
                stack.push(subLand);
            }
        }
    }

    return price;
}


} // namespace land