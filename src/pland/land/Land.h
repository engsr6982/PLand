#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/enums/LandRole.h"
#include "pland/enums/LandType.h"
#include "pland/infra/DirtyCounter.h"
#include "repo/LandContext.h"


#include "nlohmann/json.hpp"

#include <unordered_set>
#include <vector>


namespace mce {
class UUID;
};

namespace land {
namespace service {
class LandHierarchyService;
class LandManagementService;
} // namespace service

namespace observer {
class ILandObserver;
}

class Land final : public std::enable_shared_from_this<Land> {
public:
    LD_DISABLE_COPY(Land);

    LDAPI explicit Land();
    LDAPI explicit Land(LandContext ctx);
    LDAPI explicit Land(
        LandAABB const&  pos,
        LandDimid        dimid,
        bool             is3D,
        mce::UUID const& owner,
        LandPermTable    ptable = {}
    );
    LDAPI ~Land();

    template <typename... Args>
        requires std::constructible_from<Land, Args...>
    static std::shared_ptr<Land> make(Args&&... args) {
        return std::make_shared<Land>(std::forward<Args>(args)...);
    }

    LDNDAPI observer::ILandObserver* tryGetObserver() const;

    LDNDAPI LandAABB const& getAABB() const;

    LDNDAPI LandPos const& getTeleportPos() const;

    LDAPI bool setTeleportPos(LandPos const& pos);

    LDNDAPI LandID getId() const;

    LDNDAPI LandDimid getDimensionId() const;

    LDNDAPI LandPermTable const& getPermTable() const;

    LDAPI void setPermTable(LandPermTable permTable);

    /**
     * 获取领地主人的 UUID。
     *
     * ⚠️ 注意：
     * - 如果底层存储的 Owner 仍是 XUID（旧数据），此函数会返回 `mce::UUID::EMPTY()`。
     * - 在玩家上线并完成 XUID → UUID 转换之前，`getOwner()` 可能不代表真实的主人(EMPTY)。
     * - 如果需要访问原始存储值（可能是 XUID 或 UUID 字符串），请使用 `getRawOwner()`。
     */
    LDNDAPI mce::UUID const& getOwner() const;

    LDAPI void setOwner(mce::UUID const& uuid);

    [[deprecated("Use getOwner() instead, this returns raw storage string (may be XUID or UUID).")]]
    LDNDAPI std::string const& getRawOwner() const;

    /**
     * @brief 判断当前领地是否为系统所有
     * @return true/false
     */
    LDNDAPI bool isSystemOwned() const;

    LDNDAPI std::unordered_set<mce::UUID> const& getMembers() const;

    /**
     * 添加领地成员
     * @param uuid 要添加的领地成员 UUID
     * @return 是否添加成功
     * @note 此函数拒绝添加 Owner 为 Member
     */
    LDNDAPI bool addLandMember(mce::UUID const& uuid);
    LDNDAPI bool removeLandMember(mce::UUID const& uuid);

    LDAPI void clearMembers();

    LDNDAPI std::string const& getName() const;

    LDAPI void setName(std::string const& name);

    LDNDAPI int getOriginalBuyPrice() const;

    LDAPI void setOriginalBuyPrice(int price);

    /**
     * 获取领地持有类型
     */
    LDNDAPI LandHoldType getHoldType() const;
    /**
     * 获取租赁状态
     */
    LDNDAPI LeaseState getLeaseState() const;

    /**
     * 领地是否为买断制领地
     * @enum LandHoldType::Bought
     */
    LDNDAPI bool isBought() const;

    /**
     * 检查是否已被租赁
     * @enum LandHoldType::Leased
     */
    LDNDAPI bool isLeased() const;
    /**
     * 检查租赁是否有效
     * @enum LeaseState::Active
     */
    LDNDAPI bool isLeaseActive() const;
    /**
     * 检查租赁是否被冻结
     * @enum LeaseState::Frozen
     */
    LDNDAPI bool isLeaseFrozen() const;
    /**
     * 检查租赁是否已过期
     * @enum LeaseState::Expired
     */
    LDNDAPI bool isLeaseExpired() const;
    /**
     * 获取租赁开始时间
     */
    LDNDAPI time_t getLeaseStartAt() const;
    /**
     * 获取租赁结束时间
     */
    LDNDAPI time_t getLeaseEndAt() const;

    /**
     * 设置领地持有类型
     * @param type 要设置的领地持有类型
     * @note 非必要请勿直接修改数据，请在 LeasingService 中修改
     */
    LDAPI void setHoldType(LandHoldType type);
    /**
     * 设置租赁状态
     * @param state 要设置的租赁状态
     * @note 非必要请勿直接修改数据，请在 LeasingService 中修改
     */
    LDAPI void setLeaseState(LeaseState state);
    /**
     * 设置租赁开始时间
     * @param ts 要设置的租赁开始时间戳
     * @note 非必要请勿直接修改数据，请在 LeasingService 中修改
     * @note 直接调用此接口修改数据不会自动推导租赁状态，可能导致系统调度异常
     */
    LDAPI void setLeaseStartAt(time_t ts);
    /**
     * 设置租赁结束时间
     * @param ts 要设置的租赁结束时间戳
     * @note 非必要请勿直接修改数据，请在 LeasingService 中修改
     * @note 直接调用此接口修改数据不会自动推导租赁状态，可能导致系统调度异常
     */
    LDAPI void setLeaseEndAt(time_t ts);

    LDNDAPI bool is3D() const;

    LDNDAPI bool isOwner(mce::UUID const& uuid) const;

    LDNDAPI bool isMember(mce::UUID const& uuid) const;

    [[deprecated]] LDNDAPI bool isConvertedLand() const;

    [[deprecated]] LDNDAPI bool isOwnerDataIsXUID() const;

    LDNDAPI bool isCollision(BlockPos const& pos, int radius) const;

    LDNDAPI bool isCollision(BlockPos const& pos1, BlockPos const& pos2) const;

    /**
     * @brief 数据是否被修改
     * @note 当调用任意 set 方法时，数据会被标记为已修改
     * @note 调用 save 方法时，数据会被保存到数据库，并重置为未修改
     */
    LDNDAPI bool isDirty() const;

    /**
     * @brief 标记数据为已修改(计数+1)
     */
    LDAPI void                  markDirty();
    LDAPI void                  rollbackDirty();
    LDNDAPI DirtyCounter&       getDirtyCounter();
    LDNDAPI DirtyCounter const& getDirtyCounter() const;

    /**
     * @brief 获取领地类型
     */
    LDNDAPI LandType getType() const;

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
     * @brief 获取父领地 ID
     */
    LDNDAPI LandID getParentLandID() const;

    /**
     * @brief 获取子领地 ID (当前领地名下的所有子领地)
     */
    LDNDAPI std::vector<LandID> getSubLandIDs() const;

    /**
     * @brief 获取嵌套层级(相对于父领地)
     */
    LDNDAPI int getNestedLevel() const;

    /**
     * @brief 获取一个玩家在当前领地所拥有的权限类别
     */
    LDNDAPI LandPermType getPermType(mce::UUID const& uuid) const;

    /**
     * 迁移领地主人信息 (XUID -> UUID)
     * @param ownerUUID 领地主人 UUID
     */
    [[deprecated]] LDAPI void migrateOwner(mce::UUID const& ownerUUID);

    LDAPI void load(nlohmann::json& json); // 加载数据
    LDAPI nlohmann::json toJson() const;   // 导出数据

    LDAPI bool operator==(Land const& other) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    LandContext& _getContext() const;

    void _setCachedNestedLevel(int level);

    void _setLandId(LandID id);

    void _reinit(LandContext context, unsigned int dirtyDiff);

    /**
     * @brief 修改领地范围(仅限普通领地)
     * @warning 修改后务必在 LandRegistry 中刷新领地范围，否则范围不会更新
     */
    bool _setAABB(LandAABB const& newRange);

    void setObserver(observer::ILandObserver* observer);

    friend class LandRegistry;
    friend class TransactionContext;
    friend service::LandHierarchyService;
    friend service::LandManagementService;
};


} // namespace land
