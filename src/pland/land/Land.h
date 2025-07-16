#pragma once
#include "LandContext.h"
#include "ll/api/base/StdInt.h"
#include "nlohmann/json.hpp"
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/infra/DirtyCounter.h"
#include <cstdint>
#include <unordered_set>
#include <vector>


namespace land {

class Land;
class LandRegistry;

using SharedLand = std::shared_ptr<Land>; // 共享指针
using WeakLand   = std::weak_ptr<Land>;   // 弱指针

class Land final {
public:
    enum class Type {
        Ordinary = 0, // 普通领地(无父、无子)
        Parent   = 1, // 父领地(无父、有子)
        Mix      = 2, // 混合领地(有父、有子)
        Sub      = 3, // 子领地(有父、无子)
    };

private:
    LandContext  mContext;
    DirtyCounter mDirtyCounter;

    friend LandRegistry;

    SharedLand getSelfFromRegistry() const;

public:
    LD_DISALLOW_COPY(Land);

    LDAPI explicit Land();
    LDAPI explicit Land(LandContext ctx);
    LDAPI explicit Land(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner);

    template <typename... Args>
        requires std::constructible_from<Land, Args...>
    static SharedLand make(Args&&... args) {
        return std::make_shared<Land>(std::forward<Args>(args)...);
    }


    LDNDAPI LandAABB const& getAABB() const;

    /**
     * @brief 修改领地范围(仅限普通领地)
     * @param pos 领地对角坐标
     * @warning 修改后务必在 LandRegistry 中刷新领地范围，否则范围不会更新
     */
    LDNDAPI bool setAABB(LandAABB const& newRange);

    LDNDAPI LandPos const& getTeleportPos() const;

    LDAPI void setTeleportPos(LandPos const& pos);

    LDNDAPI LandID getId() const;

    LDNDAPI LandDimid getDimensionId() const;

    LDNDAPI LandPermTable const& getPermTable() const;

    LDAPI void setPermTable(LandPermTable permTable);

    LDNDAPI UUIDs const& getOwner() const;

    LDAPI void setOwner(UUIDs const& uuid);

    LDNDAPI std::vector<UUIDs> const& getMembers() const;
    LDAPI void                        addLandMember(UUIDs const& uuid);
    LDAPI void                        removeLandMember(UUIDs const& uuid);

    LDNDAPI std::string const& getName() const;

    LDAPI void setName(std::string const& name);

    LDNDAPI std::string const& getDescribe() const;

    LDAPI void setDescribe(std::string const& describe);

    LDNDAPI int getOriginalBuyPrice() const;

    LDAPI void setOriginalBuyPrice(int price);

    LDNDAPI bool is3D() const;

    LDNDAPI bool isOwner(UUIDs const& uuid) const;

    LDNDAPI bool isMember(UUIDs const& uuid) const;

    LDNDAPI bool isConvertedLand() const;

    LDNDAPI bool isOwnerDataIsXUID() const;

    LDNDAPI bool isCollision(BlockPos const& pos, int radius) const;

    LDNDAPI bool isCollision(BlockPos const& pos1, BlockPos const& pos2) const;

    /**
     * @brief 数据是否被修改
     * @note 当调用任意 set 方法时，数据会被标记为已修改
     * @note 调用 save 方法时，数据会被保存到数据库，并重置为未修改
     */
    LDNDAPI bool isDirty() const;

    /**
     * @brief 获取领地类型
     */
    LDNDAPI Type getType() const;

    /**
     * @brief 是否有父领地
     */
    LDNDAPI bool hasParentLand() const;

    /**
     * @brief 是否有子领地
     */
    LDNDAPI bool hasSubLand() const;

    /**
     * @brief 是否为子领地(有父领地、无子领地)
     */
    LDNDAPI bool isSubLand() const;

    /**
     * @brief 是否为父领地(有子领地、无父领地)
     */
    LDNDAPI bool isParentLand() const;

    /**
     * @brief 是否为混合领地(有父领地、有子领地)
     */
    LDNDAPI bool isMixLand() const;

    /**
     * @brief 是否为普通领地(无父领地、无子领地)
     */
    LDNDAPI bool isOrdinaryLand() const;

    /**
     * @brief 是否可以创建子领地
     * 如果满足嵌套层级限制，则可以创建子领地
     */
    LDNDAPI bool canCreateSubLand() const;

    /**
     * @brief 获取父领地
     */
    LDNDAPI SharedLand getParentLand() const;

    /**
     * @brief 获取子领地
     */
    LDNDAPI std::vector<SharedLand> getSubLands() const;

    /**
     * @brief 获取嵌套层级(相对于父领地)
     */
    LDNDAPI int getNestedLevel() const;

    /**
     * @brief 获取根领地(即最顶层的普通领地 isOrdinaryLand() == true)
     */
    LDNDAPI SharedLand getRootLand() const;

    /**
     * @brief 获取从当前领地的根领地出发的所有子领地（包含根和当前领地）
     */
    LDNDAPI std::unordered_set<SharedLand> getFamilyTree() const;

    /**
     * @brief 获取当前领地及其所有上级父领地（包含自身）
     */
    LDNDAPI std::unordered_set<SharedLand> getSelfAndAncestors() const;

    /**
     * @brief 获取一个玩家在当前领地所拥有的权限类别
     */
    LDNDAPI LandPermType getPermType(UUIDs const& uuid) const;

    LDAPI void updateXUIDToUUID(UUIDs const& ownerUUID); // xuid -> uuid

    LDAPI void load(nlohmann::json& json); // 加载数据
    LDAPI nlohmann::json dump() const;     // 导出数据

    /**
     * @brief 保存数据(保存到数据库)
     * @param force 是否强制保存(即使数据未修改)
     * @note 此函数仅在数据有修改时才会保存，否则不会保存
     */
    LDAPI void save(bool force = false);

    LDAPI bool operator==(SharedLand const& other) const;

public:
    using RecursionCalculationPriceHandle = std::function<bool(SharedLand const& land, llong& price)>;

    /**
     * @brief 递归计算领地总价值(子领地)
     * @param land 领地数据
     * @param handle 处理函数，返回false则终止递归 (默认)
     * @return 总价值
     */
    LDAPI static llong
    calculatePriceRecursively(SharedLand const& land, RecursionCalculationPriceHandle const& handle = {});
};


} // namespace land