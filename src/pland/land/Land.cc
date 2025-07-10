#include "pland/land/Land.h"
#include "pland/Global.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"
#include "pland/utils/JSON.h"
#include <stack>
#include <vector>



namespace land {

// getters
LandAABB const&           Land::getLandPos() const { return mPos; }
LandID                    Land::getLandID() const { return mLandID; }
LandDimid                 Land::getLandDimid() const { return mLandDimid; }
LandPermTable&            Land::getLandPermTable() { return mLandPermTable; }
LandPermTable const&      Land::getLandPermTable() const { return mLandPermTable; }
LandPermTable const&      Land::getLandPermTableConst() const { return mLandPermTable; }
UUIDs const&              Land::getLandOwner() const { return mLandOwner; }
std::vector<UUIDs> const& Land::getLandMembers() const { return mLandMembers; }
std::string const&        Land::getLandName() const { return mLandName; }
std::string const&        Land::getLandDescribe() const { return mLandDescribe; }
int                       Land::getSalePrice() const { return mSalePrice; }

// setters
bool Land::setIs3DLand(bool is3DLand) { return mIs3DLand = is3DLand; }
bool Land::setSaleing(bool saleing) { return mIsSaleing = saleing; }
bool Land::setLandOwner(UUIDs const& uuid) {
    mLandOwner = UUIDs(uuid);
    return true;
}
bool Land::setLandName(std::string const& name) {
    mLandName = std::string(name);
    return true;
}
bool Land::setLandDescribe(std::string const& describe) {
    mLandDescribe = std::string(describe);
    return true;
}
bool Land::_setLandPos(LandAABB const& pos) {
    mPos = LandAABB(pos);
    return true;
}
bool Land::setSalePrice(int price) {
    mSalePrice = price;
    return true;
}

bool Land::addLandMember(UUIDs const& uuid) {
    mLandMembers.push_back(UUIDs(uuid));
    return true;
}
bool Land::removeLandMember(UUIDs const& uuid) {
    mLandMembers.erase(std::remove(mLandMembers.begin(), mLandMembers.end(), uuid), mLandMembers.end());
    return true;
}

// helpers
bool Land::is3DLand() const { return mIs3DLand; }
bool Land::isSaleing() const { return mIsSaleing; }
bool Land::isLandOwner(UUIDs const& uuid) const { return mLandOwner == uuid; }
bool Land::isLandMember(UUIDs const& uuid) const {
    return std::find(mLandMembers.begin(), mLandMembers.end(), uuid) != mLandMembers.end();
}

bool Land::hasParentLand() const { return this->mParentLandID != LandID(-1); }
bool Land::hasSubLand() const { return !this->mSubLandIDs.empty(); }
bool Land::isSubLand() const { return this->mParentLandID != LandID(-1) && this->mSubLandIDs.empty(); }
bool Land::isParentLand() const { return this->mParentLandID == LandID(-1) && !this->mSubLandIDs.empty(); }
bool Land::isMixLand() const { return this->mParentLandID != LandID(-1) && !this->mSubLandIDs.empty(); }
bool Land::isOrdinaryLand() const { return this->mParentLandID == LandID(-1) && this->mSubLandIDs.empty(); }
bool Land::canCreateSubLand() const {
    auto nestedLevel = getNestedLevel();
    return nestedLevel < Config::cfg.land.subLand.maxNested && nestedLevel < GlobalSubLandMaxNestedLevel
        && static_cast<int>(this->mSubLandIDs.size()) < Config::cfg.land.subLand.maxSubLand;
}

Land_sptr Land::getParentLand() const {
    if (isParentLand() || !hasParentLand()) {
        return nullptr;
    }
    return LandRegistry::getInstance().getLand(this->mParentLandID);
}

std::vector<Land_sptr> Land::getSubLands() const {
    if (!hasSubLand()) {
        return {};
    }
    return LandRegistry::getInstance().getLands(this->mSubLandIDs);
}
int Land::getNestedLevel() const {
    if (!hasParentLand()) {
        return 0;
    }

    std::stack<Land_sptr> stack;
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
Land_sptr Land::getRootLand() const {
    if (!hasParentLand()) {
        return LandRegistry::getInstance().getLand(this->mLandID); // 如果是父领地，直接返回自己
    }

    Land_sptr root = getParentLand();
    while (root->hasParentLand()) {
        root = root->getParentLand();
    }

    return root;
}

bool Land::isRadiusInLand(BlockPos const& pos, int radius) const {
    BlockPos minPos(pos.x - radius, mIs3DLand ? pos.y - radius : mPos.min.y, pos.z - radius);
    BlockPos maxPos(pos.x + radius, mIs3DLand ? pos.y + radius : mPos.max.y, pos.z + radius);
    return isAABBInLand(minPos, maxPos);
}

bool Land::isAABBInLand(BlockPos const& pos1, BlockPos const& pos2) const {
    return LandAABB::isCollision(
        mPos,
        LandAABB{
            LandPos{pos1.x, pos1.y, pos1.z},
            LandPos{pos2.x, pos2.y, pos2.z}
    }
    );
}


LandPermType Land::getPermType(UUIDs const& uuid) const {
    if (uuid.empty()) return LandPermType::Guest; // empty uuid is guest
    if (isLandOwner(uuid)) return LandPermType::Owner;
    if (isLandMember(uuid)) return LandPermType::Member;
    return LandPermType::Guest;
}

nlohmann::json Land::toJSON() const { return JSON::structTojson(*this); }
void           Land::load(nlohmann::json& json) { JSON::jsonToStruct(json, *this); }

bool Land::operator==(Land_sptr const& other) const { return mLandID == other->mLandID; }


// static
Land_sptr Land::make() { return std::make_shared<Land>(); }
Land_sptr Land::make(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner) {
    auto ptr     = std::make_shared<Land>();
    ptr->mLandID = LandID(-1);
    ptr->_setLandPos(pos);
    ptr->mLandDimid = dimid;
    ptr->setIs3DLand(is3D);
    ptr->setLandOwner(owner);
    return ptr;
}

llong Land::calculatePriceRecursively(Land_sptr const& land, RecursionCalculationPriceHandle const& handle) {
    std::stack<Land_sptr> stack;
    stack.push(land);

    llong price = 0;

    while (!stack.empty()) {
        Land_sptr current = stack.top();
        stack.pop();

        if (handle) {
            if (!handle(current, price)) break; // if handle return false, break
        } else {
            price += current->mOriginalBuyPrice;
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