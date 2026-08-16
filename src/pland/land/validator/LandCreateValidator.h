#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"

#include "ll/api/Expected.h"

#include <concepts>
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
 */
class LandCreateValidator {
public:
    LandCreateValidator() = delete;

    enum class ErrorCode {
        Undefined                     = 0, // 未定义
        LandCountLimitExceeded        = 1, // 领地数量超过限制
        LandInForbiddenRange          = 2, // 领地位于禁止范围内
        LandRangeTooSmall             = 3, // 领地范围太小
        LandRangeTooLarge             = 4, // 领地范围太大
        LandHeightTooSmall            = 5, // 领地高度太低
        LandRangeConflict             = 6, // 领地范围冲突
        LandSpacingTooSmall           = 7, // 领地间距太小
        SubLandOutOfParentLandRange   = 8, // 子领地超出父领地范围
        LandOutOfDimensionHeightRange = 9, // 领地高度超出范围(维度高度)
        LandInLeaseOnlyRange          = 10 // 领地位于租赁范围内
    };

    // 由于 LeviLamina 限制，Error::isA<T> 只能精确匹配匹配类型
    // 所以这里使用自定义的异常上下文来承载错误信息
    struct ErrorContext {
        ErrorCode code{ErrorCode::Undefined};

        explicit ErrorContext(ErrorCode code) : code(code) {}
        LDAPI virtual ~ErrorContext() = default;

        virtual std::string translateError(std::string const& localeCode) const = 0;
    };

    struct LandCountExceededContext : ErrorContext {
        int count;
        int maxCount;
        LandCountExceededContext(int count, int maxCount);
        std::string translateError(std::string const& localeCode) const override;
    };

    struct LandInForbiddenRangeContext : ErrorContext {
        LandAABB range;
        LandAABB forbiddenRange;
        LandInForbiddenRangeContext(LandAABB const& range, LandAABB const& forbiddenRange);
        std::string translateError(std::string const& localeCode) const override;
    };

    struct LandInLeaseOnlyRangeContext : ErrorContext {
        LandAABB range;
        LandAABB leaseOnlyRange;
        LandInLeaseOnlyRangeContext(LandAABB const& range, LandAABB const& leaseOnlyRange);
        std::string translateError(std::string const& localeCode) const override;
    };

    struct LandRangeErrorContext : ErrorContext {
        int limitSize;

        LandRangeErrorContext(ErrorCode code, int limit);
        std::string translateError(std::string const& localeCode) const override;
    };

    struct LandHeightErrorContext : ErrorContext {
        int actualHeight; // 实际高度
        int limitHeight;  // 限制高度

        LandHeightErrorContext(ErrorCode code, int actual, int limit);
        std::string translateError(std::string const& localeCode) const override;
    };

    struct LandRangeConflictContext : ErrorContext {
        LandAABB              range;
        std::shared_ptr<Land> conflictLand;
        LandRangeConflictContext(LandAABB const& range, std::shared_ptr<Land> conflictLand);
        std::string translateError(std::string const& localeCode) const override;
    };

    struct LandSpacingContext : ErrorContext {
        int                   spacing;
        int                   minSpacing;
        std::shared_ptr<Land> conflictLand;
        LandSpacingContext(int spacing, int minSpacing, std::shared_ptr<Land> conflictLand);
        std::string translateError(std::string const& localeCode) const override;
    };

    struct SubLandNotInParentContext : ErrorContext {
        std::shared_ptr<Land> parent;
        LandAABB              subRange;
        SubLandNotInParentContext(std::shared_ptr<Land> parent, LandAABB const& subRange);
        std::string translateError(std::string const& localeCode) const override;
    };


    struct ValidateError : ll::ErrorInfoBase {
        std::unique_ptr<ErrorContext> mContext;

        explicit ValidateError(std::unique_ptr<ErrorContext> context) : mContext(std::move(context)) {}

        LDAPI std::string message(std::string_view locale) const noexcept final;

        LDAPI void sendTo(Player& player) const;
        LDAPI std::string translateError(std::string const& localeCode) const;
    };

    template <std::derived_from<ErrorContext> T, typename... Args>
        requires std::constructible_from<T, Args...>
    static inline auto makeError(Args&&... args) {
        return ll::makeError<ValidateError>(std::make_unique<T>(std::forward<Args>(args)...));
    }

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
    validateChangeLandRange(LandRegistry& registry, std::shared_ptr<Land> land, LandAABB newRange);

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
     */
    LDNDAPI static ll::Expected<> ensurePlayerLandCountNotExceeded(LandRegistry& registry, mce::UUID const& uuids);

    /**
     * @brief 确保领地不在禁止范围内
     */
    LDNDAPI static ll::Expected<> ensureLandNotInForbiddenRange(LandAABB const& range, LandDimid dimid);

    /**
     * @brief 确保领地不再仅租赁区域内
     */
    LDNDAPI static ll::Expected<> ensureLandNotInLeaseOnlyRange(LandAABB const& range, LandDimid dimid);

    /**
     * @brief 确保领地范围合法
     */
    LDNDAPI static ll::Expected<> ensureLandRangeIsLegal(LandAABB const& range, LandDimid dimid, bool is3D);

    /**
     * @brief 确保领地范围无冲突
     * @param newRange 新范围，若为空则使用 land 的范围
     */
    LDNDAPI static ll::Expected<> ensureNoLandRangeConflict(
        LandRegistry&                registry,
        std::shared_ptr<Land> const& land,
        std::optional<LandAABB>      newRange = std::nullopt
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
        LandAABB const&                subRange
    );
};


} // namespace land
