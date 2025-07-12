#pragma once
#include "LandContext.h"
#include "ll/api/base/StdInt.h"
#include "nlohmann/json.hpp"
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/infra/DirtyCounter.h"
#include <cstdint>
#include <vector>


namespace land {

class Land;
class LandRegistry;

using SharedLand = std::shared_ptr<Land>; // 共享指针
using WeakLand   = std::weak_ptr<Land>;   // 弱指针

class Land final {
public:
    enum class Type {
        Normal = 0, // 普通领地(无父、无子)
        Parent = 1, // 父领地(无父、有子)
        Mix    = 2, // 混合领地(有父、有子)
        Sub    = 3, // 子领地(有父、无子)
    };

private:
    LandContext  mContext;
    DirtyCounter mDirtyCounter;

    friend LandRegistry;

public:
    LD_DISALLOW_COPY(Land);

    LDAPI explicit Land();
    LDAPI explicit Land(LandContext ctx);
    LDAPI explicit Land(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner);


    LDNDAPI LandAABB const& getAABB() const;

    /**
     * @brief 修改领地范围(仅限普通领地)
     * @param pos 领地对角坐标
     * @warning 修改后务必在 LandRegistry 中刷新领地范围，否则范围不会更新
     */
    LDAPI void setAABB(LandAABB const& pos);

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

    LDNDAPI bool isDirty() const; // 是否需要保存(数据有变动)

    /**
     * @brief 获取领地类型
     */
    LDNDAPI Type getType() const;

    LDNDAPI bool hasParentLand() const; // 是否有父领地
    LDNDAPI bool hasSubLand() const;    // 是否有子领地

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


    LDNDAPI LandPermType getPermType(UUIDs const& uuid) const;

    LDAPI void updateXUIDToUUID(UUIDs const& ownerUUID); // xuid -> uuid

    LDAPI void load(nlohmann::json& json);         // 加载数据
    LDAPI nlohmann::json dump() const;             // 导出数据
    LDAPI void           save(bool force = false); // 保存数据(保存到数据库)

    LDAPI bool operator==(SharedLand const& other) const;

public:
    LDNDAPI static SharedLand make();
    LDNDAPI static SharedLand make(LandContext ctx);
    LDNDAPI static SharedLand make(LandAABB const& pos, LandDimid dimid, bool is3D, UUIDs const& owner); // 新建领地数据


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