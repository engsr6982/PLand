#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"

#include <mc/deps/core/math/Vec3.h>

#include "ll/api/Expected.h"

#include <memory>
#include <optional>

class Player;
namespace mce {
class UUID;
}

namespace land {
struct LandResizeSettlement;
class Land;
class OrdinaryLandCreateSelector;
class SubLandCreateSelector;
class LandResizeSelector;
class LandRegistry;
} // namespace land
namespace land::service {
class LandHierarchyService;
class LandPriceService;
class SelectionService;
} // namespace land::service

namespace land {
namespace service {

enum class DeletePolicy : uint8_t {
    CurrentOnly,      // 只删当前（普通 or 子）
    Recursive,        // 递归删除子领地
    PromoteChildren,  // 删除当前，子领地提升
    TransferChildren, // 删除当前，子领地转移
};

class LandManagementService {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    LandManagementService(
        LandRegistry&         registry,
        SelectionService&     selectionService,
        LandHierarchyService& hierarchyService,
        LandPriceService&     priceService
    );
    ~LandManagementService();

    LD_DISABLE_COPY_AND_MOVE(LandManagementService);

    /**
     * 请求创建普通领地
     * @param player 玩家
     * @param is3D 领地类别
     */
    LDNDAPI ll::Expected<> requestCreateOrdinaryLand(Player& player, bool is3D) const;

    /**
     * 请求创建子领地
     * @param player 玩家
     */
    LDNDAPI ll::Expected<> requestCreateSubLand(Player& player);

    LDNDAPI ll::Expected<> requestCreateSubLand(Player& player, std::shared_ptr<Land> const& land) const;

    /**
     * 购买普通领地
     * @param player 玩家
     * @param selector 选区
     * @param money 需支付经济
     * @return 领地对象
     */
    LDNDAPI ll::Expected<std::shared_ptr<Land>>
            buyLand(Player& player, OrdinaryLandCreateSelector* selector, int64_t money);

    /**
     * 购买子领地
     * @param player 玩家
     * @param selector 选区
     * @param money 需要支付的经济
     * @return 子领地对象
     */
    LDNDAPI ll::Expected<std::shared_ptr<Land>> buyLand(Player& player, SubLandCreateSelector* selector, int64_t money);

    /**
     * 处理领地范围变更
     * @param player 玩家
     * @param selector 选区
     * @param settlement 结算数据
     */
    LDNDAPI ll::Expected<>
            handleChangeRange(Player& player, LandResizeSelector* selector, LandResizeSettlement const& settlement);

    /**
     * 确保玩家领地数量不超过上限
     * @param uuid 玩家UUID
     * @param locale 用于 i18n 错误消息
     */
    LDNDAPI ll::Expected<> ensurePlayerLandCountLimit(mce::UUID const& uuid, std::string_view locale = {}) const;

    LDNDAPI ll::Expected<> setLandTeleportPos(Player& player, std::shared_ptr<Land> const& land, Vec3 point);

    LDNDAPI ll::Expected<> deleteLand(Player& player, std::shared_ptr<Land> ptr, DeletePolicy policy);

    LDNDAPI ll::Expected<> setLandName(Player& player, std::shared_ptr<Land> const& land, std::string name);

    LDNDAPI ll::Expected<> transferLand(Player& player, std::shared_ptr<Land> const& land, Player& target);
    LDNDAPI ll::Expected<> transferLand(Player& player, std::shared_ptr<Land> const& land, mce::UUID const& target);

    LDNDAPI ll::Expected<> requestChangeRange(Player& player, std::shared_ptr<Land> const& land);

    LDNDAPI ll::Expected<> addMember(Player& player, std::shared_ptr<Land> const& land, mce::UUID const& target);
    LDNDAPI ll::Expected<> removeMember(Player& player, std::shared_ptr<Land> const& land, mce::UUID const& target);


private:
    ll::Expected<>
    _playerChangeMember(Player& player, std::shared_ptr<Land> const& land, mce::UUID const& target, bool isAdd);

    ll::Expected<> _ensureChangeRangelegal(
        std::shared_ptr<Land> const&    land,
        LandAABB const&                 newRange,
        std::optional<std::string_view> localeCode = std::nullopt
    );

    ll::Expected<> _applyRangeChange(Player& player, std::shared_ptr<Land> const& land, LandAABB const& newRange);

    ll::Expected<std::shared_ptr<Land>>
    _payMoneyAndCreateOrdinaryLand(Player& player, OrdinaryLandCreateSelector* selector, int64_t money);

    ll::Expected<> _addOrdinaryLand(Player& player, std::shared_ptr<Land> ptr);

    ll::Expected<std::shared_ptr<Land>>
    _payMoneyAndCreateSubLand(Player& player, SubLandCreateSelector* selector, int64_t money);

    ll::Expected<> _ensureAndAttachSubLand(Player& player, std::shared_ptr<Land> parent, std::shared_ptr<Land> sub);

    ll::Expected<> _processResizeSettlement(Player& player, LandResizeSettlement const& settlement);

    ll::Expected<> _ensureLandWithDeletePolicy(Player& player, std::shared_ptr<Land> const& land, DeletePolicy policy);

    ll::Expected<> _processDeleteLand(Player& player, std::shared_ptr<Land> const& land, DeletePolicy policy);

    ll::Expected<> _processLandRefund(Player& player, std::shared_ptr<Land> const& land, bool isSingle);
};

} // namespace service
} // namespace land
