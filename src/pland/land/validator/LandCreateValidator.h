#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"

#include "ll/api/Expected.h"

#include <memory>
#include <optional>

class Player;
namespace mce {
class UUID;
} // namespace mce

namespace land {

class Land;
class LandRegistry;
namespace service {
class LandHierarchyService;
}

/**
 * @brief 领地创建验证器
 * @note 所有验证函数接受可选的 locale 参数用于 i18n 错误消息，
 *       默认使用内置语言（简体中文）。返回 StringError。
 */
class LandCreateValidator {
public:
    LandCreateValidator() = delete;

public:
    /**
     * 验证创建普通领地
     * @param registry 领地注册表
     * @param player 创建此领地的玩家
     * @param land 创建的领地对象
     */
    LDNDAPI static ll::Expected<>
    validateCreateOrdinaryLand(LandRegistry& registry, Player& player, std::shared_ptr<Land> land);

    LDNDAPI static ll::Expected<>
    validateChangeLandRange(LandRegistry& registry, std::shared_ptr<Land> land, LandAABB newRange, std::string_view locale = {});

    LDNDAPI static ll::Expected<> validateCreateSubLand(
        Player&                        player,
        std::shared_ptr<Land>          land,
        LandAABB const&                subRange,
        LandRegistry&                  registry,
        service::LandHierarchyService& service
    );

public:
    /**
     * @brief 确保玩家领地数量未超限
     * @param locale 用于 i18n 错误消息，默认使用内置语言
     */
    LDNDAPI static ll::Expected<> ensurePlayerLandCountNotExceeded(
        LandRegistry&      registry,
        mce::UUID const&   uuids,
        std::string_view   locale = {}
    );

    /**
     * @brief 确保领地不在禁止范围内
     */
    LDNDAPI static ll::Expected<>
    ensureLandNotInForbiddenRange(LandAABB const& range, LandDimid dimid, std::string_view locale = {});

    /**
     * @brief 确保领地不在仅租赁区域内
     */
    LDNDAPI static ll::Expected<>
    ensureLandNotInLeaseOnlyRange(LandAABB const& range, LandDimid dimid, std::string_view locale = {});

    /**
     * @brief 确保领地范围合法（尺寸、高度）
     */
    LDNDAPI static ll::Expected<>
    ensureLandRangeIsLegal(LandAABB const& range, LandDimid dimid, bool is3D, std::string_view locale = {});

    /**
     * @brief 确保领地范围无冲突
     * @param newRange 新范围，若为空则使用 land 的范围
     */
    LDNDAPI static ll::Expected<> ensureNoLandRangeConflict(
        LandRegistry&                registry,
        std::shared_ptr<Land> const& land,
        std::optional<LandAABB>      newRange = std::nullopt,
        std::string_view             locale   = {}
    );

    /**
     * @brief 确保子领地位置合法(相对于父领地)
     * @param hierarchyService 领地层级服务
     * @param land 父领地 (相对于 sub 的父领地)
     * @param subRange 子领地
     * @note 满足下列所有条件:
     * @note 1. 子领地必须完全包含于父领地范围内。
     * @note 2. 子领地不能与父领地的其它子孙领地冲突（除直系父领地）。
     * @note 3. 子领地与其它家族成员的距离不能小于最小间距要求。
     */
    LDNDAPI static ll::Expected<> ensureSubLandPositionIsLegal(
        service::LandHierarchyService& hierarchyService,
        std::shared_ptr<Land> const&   land,
        LandAABB const&                subRange,
        std::string_view               locale = {}
    );
};


} // namespace land
