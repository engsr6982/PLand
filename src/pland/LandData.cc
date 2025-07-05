#include "pland/LandData.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/utils/JSON.h"
#include <stack>
#include <vector>

namespace land {

// getters
LandAABB const&           LandData::getLandPos() const { return mPos; }
LandID                    LandData::getLandID() const { return mLandID; }
LandDimid                 LandData::getLandDimid() const { return mLandDimid; }
LandPermTable&            LandData::getLandPermTable() { return mLandPermTable; }
LandPermTable const&      LandData::getLandPermTable() const { return mLandPermTable; }
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
bool LandData::_setLandPos(LandAABB const& pos) {
    mPos = LandAABB(pos);
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

bool LandData::hasParentLand() const { return this->mParentLandID != LandID(-1); }
bool LandData::hasSubLand() const { return !this->mSubLandIDs.empty(); }
bool LandData::isSubLand() const { return this->mParentLandID != LandID(-1) && this->mSubLandIDs.empty(); }
bool LandData::isParentLand() const { return this->mParentLandID == LandID(-1) && !this->mSubLandIDs.empty(); }
bool LandData::isMixLand() const { return this->mParentLandID != LandID(-1) && !this->mSubLandIDs.empty(); }
bool LandData::isOrdinaryLand() const { return this->mParentLandID == LandID(-1) && this->mSubLandIDs.empty(); }
bool LandData::canCreateSubLand() const {
    auto nestedLevel = getNestedLevel();
    return nestedLevel < Config::cfg.land.subLand.maxNested && nestedLevel < GlobalSubLandMaxNestedLevel
        && static_cast<int>(this->mSubLandIDs.size()) < Config::cfg.land.subLand.maxSubLand;
}

LandData_sptr LandData::getParentLand() const {
    if (isParentLand() || !hasParentLand()) {
        return nullptr;
    }
    return PLand::getInstance().getLand(this->mParentLandID);
}

std::vector<LandData_sptr> LandData::getSubLands() const {
    if (!hasSubLand()) {
        return {};
    }
    return PLand::getInstance().getLands(this->mSubLandIDs);
}
int LandData::getNestedLevel() const {
    if (!hasParentLand()) {
        return 0;
    }

    std::stack<LandData_sptr> stack;
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
LandData_sptr LandData::getRootLand() const {
    if (!hasParentLand()) {
        return PLand::getInstance().getLand(this->mLandID); // 如果是父领地，直接返回自己
    }

    LandData_sptr root = getParentLand();
    while (root->hasParentLand()) {
        root = root->getParentLand();
    }

    return root;
}

bool LandData::isRadiusInLand(BlockPos const& pos, int radius) const {
    BlockPos minPos(pos.x - radius, mIs3DLand ? pos.y - radius : mPos.min.y, pos.z - radius);
    BlockPos maxPos(pos.x + radius, mIs3DLand ? pos.y + radius : mPos.max.y, pos.z + radius);
    return isAABBInLand(minPos, maxPos);
}

bool LandData::isAABBInLand(BlockPos const& pos1, BlockPos const& pos2) const {
    return LandAABB::isCollision(
        mPos,
        LandAABB{
            LandPos{pos1.x, pos1.y, pos1.z},
            LandPos{pos2.x, pos2.y, pos2.z}
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
void           LandData::load(nlohmann::json& json) { JSON::jsonToStruct(json, *this); }

bool LandData::operator==(LandData_sptr const& other) const { return mLandID == other->mLandID; }


// static
LandData_sptr LandData::make() { return std::make_shared<LandData>(); }
LandData_sptr LandData::make(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner) {
    auto ptr     = std::make_shared<LandData>();
    ptr->mLandID = LandID(-1);
    ptr->_setLandPos(pos);
    ptr->mLandDimid = dimid;
    ptr->setIs3DLand(is3D);
    ptr->setLandOwner(owner);
    return ptr;
}

} // namespace land