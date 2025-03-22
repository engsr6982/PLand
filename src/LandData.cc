#include "pland/LandData.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/utils/JSON.h"
#include <vector>

namespace land {

// getters
LandPos const&            LandData::getLandPos() const { return mPos; }
LandID                    LandData::getLandID() const { return mLandID; }
LandDimid                 LandData::getLandDimid() const { return mLandDimid; }
LandPermTable&            LandData::getLandPermTable() { return mLandPermTable; }
LandPermTable const&      LandData::getLandPermTableConst() const { return mLandPermTable; }
UUIDs const&              LandData::getLandOwner() const { return mLandOwner; }
std::vector<UUIDs> const& LandData::getLandMembers() const { return mLandMembers; }
std::string const&        LandData::getLandName() const { return mLandName; }
std::string const&        LandData::getLandDescribe() const { return mLandDescribe; }
int                       LandData::getSalePrice() const { return mSalePrice; }

// setters
bool LandData::setIs3DLand(bool is3DLand) { return mIs3DLand = is3DLand; }
bool LandData::setSaleing(bool saleing) { return mIsSaleing = saleing; }
bool LandData::setLandOwner(UUIDs const& uuid) {
    mLandOwner = UUIDs(uuid);
    return true;
}
bool LandData::setLandName(std::string const& name) {
    mLandName = std::string(name);
    return true;
}
bool LandData::setLandDescribe(std::string const& describe) {
    mLandDescribe = std::string(describe);
    return true;
}
bool LandData::_setLandPos(LandPos const& pos) {
    mPos = LandPos(pos);
    return true;
}
bool LandData::setSalePrice(int price) {
    mSalePrice = price;
    return true;
}

bool LandData::addLandMember(UUIDs const& uuid) {
    mLandMembers.push_back(UUIDs(uuid));
    return true;
}
bool LandData::removeLandMember(UUIDs const& uuid) {
    mLandMembers.erase(std::remove(mLandMembers.begin(), mLandMembers.end(), uuid), mLandMembers.end());
    return true;
}

// helpers
bool LandData::is3DLand() const { return mIs3DLand; }
bool LandData::isSaleing() const { return mIsSaleing; }
bool LandData::isLandOwner(UUIDs const& uuid) const { return mLandOwner == uuid; }
bool LandData::isLandMember(UUIDs const& uuid) const {
    return std::find(mLandMembers.begin(), mLandMembers.end(), uuid) != mLandMembers.end();
}

bool LandData::isSubLand() const { return this->mParentLandID != LandID(-1) && this->mSubLandIDs.empty(); }
bool LandData::isParentLand() const { return this->mParentLandID == LandID(-1) && !this->mSubLandIDs.empty(); }
bool LandData::isMixLand() const { return isSubLand() && isParentLand(); }
bool LandData::isOrdinaryLand() const { return !isMixLand(); }
bool LandData::canCreateSubLand() const {
    auto nestedLevel = getNestedLevel();
    return nestedLevel < Config::cfg.land.subLand.maxNested && nestedLevel < GlobalSubLandMaxNestedLevel;
}

LandData_sptr LandData::getParentLand() const {
    if (isParentLand()) {
        return nullptr;
    }
    return PLand::getInstance().getLand(this->mParentLandID);
}

std::vector<LandData_sptr> LandData::getSubLands() const {
    if (!isParentLand()) {
        return {}; // 不是父领地，没有子领地
    }
    return PLand::getInstance().getLands(this->mSubLandIDs);
}
int LandData::getNestedLevel() const {
    if (isParentLand()) {
        return 0;
    }
    auto parentLand = getParentLand();
    if (!parentLand) {
        return 0;
    }
    return parentLand->getNestedLevel() + 1;
}

bool LandData::isRadiusInLand(BlockPos const& pos, int radius) const {
    BlockPos minPos(pos.x - radius, mIs3DLand ? pos.y - radius : mPos.mMin_A.y, pos.z - radius);
    BlockPos maxPos(pos.x + radius, mIs3DLand ? pos.y + radius : mPos.mMax_B.y, pos.z + radius);
    return isAABBInLand(minPos, maxPos);
}

bool LandData::isAABBInLand(BlockPos const& pos1, BlockPos const& pos2) const {
    return LandPos::isCollision(
        mPos,
        LandPos{
            PosBase{pos1.x, pos1.y, pos1.z},
            PosBase{pos2.x, pos2.y, pos2.z}
    }
    );
}


LandPermType LandData::getPermType(UUIDs const& uuid) const {
    if (uuid.empty()) return LandPermType::Guest; // empty uuid is guest
    if (isLandOwner(uuid)) return LandPermType::Owner;
    if (isLandMember(uuid)) return LandPermType::Member;
    return LandPermType::Guest;
}

nlohmann::json LandData::toJSON() const { return JSON::structTojson(*this); }

bool LandData::operator==(LandData_sptr const& other) const { return mLandID == other->mLandID; }


// static
LandData_sptr LandData::make() { return std::make_shared<LandData>(); }
LandData_sptr LandData::make(LandPos const& pos, LandDimid dimid, bool is3D, UUIDs const& owner) {
    auto ptr     = std::make_shared<LandData>();
    ptr->mLandID = LandID(-1);
    ptr->_setLandPos(pos);
    ptr->mLandDimid = dimid;
    ptr->setIs3DLand(is3D);
    ptr->setLandOwner(owner);
    return ptr;
}

} // namespace land