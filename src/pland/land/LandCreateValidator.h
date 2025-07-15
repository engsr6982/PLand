#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/Land.h"
#include <optional>


namespace land {


/**
 * @brief 领地创建验证器
 */
class LandCreateValidator {
public:
    LandCreateValidator() = delete;

    enum class ErrorCode {
        Undefined,                   // 未定义
        LandCountLimitExceeded,      // 领地数量超过限制
        LandInForbiddenRange,        // 领地位于禁止范围内
        LandRangeTooSmall,           // 领地范围太小
        LandRangeTooLarge,           // 领地范围太大
        LandHeightTooSmall,          // 领地高度太低
        LandHeightTooLarge,          // 领地高度太高
        LandRangeWithOtherCollision, // 领地范围与其他领地冲突
        LandSpacingTooSmall,         // 领地间距太小
        SubLandNotInParent,          // 子领地不在父领地范围内
    };

    struct ErrorContext {
        ErrorCode code{ErrorCode::Undefined};

        // Optional context fields
        int currentCount{}; // LandCountLimitExceeded
        int maxCount{};

        LandAABB currentRange{}; // 通用范围
        LandAABB forbiddenRange{};

        int minRange{};
        int maxRange{};
        int minHeight{};
        int maxHeight{};

        SharedLand conflictLand{nullptr};
        int        spacing{};
        int        minSpacing{};

        LDAPI static ErrorContext undefined();
        LDAPI static ErrorContext landCountLimitExceeded(int currentCount, int maxCount);
        LDAPI static ErrorContext landInForbiddenRange(LandAABB const& range, LandAABB const& forbiddenRange);
        LDAPI static ErrorContext landRangeTooSmall(LandAABB const& range, int minRange);
        LDAPI static ErrorContext landRangeTooLarge(LandAABB const& range, int maxRange);
        LDAPI static ErrorContext landHeightTooSmall(LandAABB const& range, int minHeight);
        LDAPI static ErrorContext landHeightTooLarge(LandAABB const& range, int maxHeight);
        LDAPI static ErrorContext landRangeWithOtherCollision(SharedLand const& conflictLand);
        LDAPI static ErrorContext landSpacingTooSmall(SharedLand const& conflictLand, int spacing, int minSpacing);
        LDAPI static ErrorContext subLandNotInParent(SharedLand const& land, LandAABB const& subRange);
    };

    using ValidateResult = Result<void, ErrorContext>;
    LDAPI static void sendErrorMessage(Player& player, ErrorContext const& ctx);

public:
    LDNDAPI static ValidateResult validateCreateOrdinaryLand(Player& player, SharedLand land); // 普通领地

    LDNDAPI static ValidateResult validateChangeLandRange(SharedLand land, LandAABB newRange); // 改变范围

    LDNDAPI static ValidateResult
    validateCreateSubLand(Player& player, SharedLand land, LandAABB const& subRange); // 创建子领地

public:
    /**
     * @brief 玩家领地数量是否超过限制
     */
    LDNDAPI static ValidateResult isPlayerLandCountLimitExceeded(UUIDs const& uuids);

    /**
     * @brief 领地是否在禁止范围内
     */
    LDNDAPI static ValidateResult isLandInForbiddenRange(LandAABB const& range, LandDimid dimid);

    /**
     * @brief 领地范围是否合法
     */
    LDNDAPI static ValidateResult isLandRangeLegal(LandAABB const& range, LandDimid dimid, bool is3D);

    /**
     * @brief 领地范围与其他领地是否冲突
     * @param newRange 新范围，若为空则使用 land 的范围
     */
    LDNDAPI static ValidateResult
    isLandRangeWithOtherCollision(SharedLand const& land, std::optional<LandAABB> newRange = std::nullopt);

    /**
     * @brief 验证子领地位置是否合法(相对于父领地)
     * @param land 父领地 (相对于 sub 的父领地)
     * @param sub 子领地
     * @note 满足下列所有条件:
     * @note 1. 子领地必须完全包含于父领地范围内。
     * @note 2. 子领地不能与父领地的其它子孙领地冲突（除直系父领地）。
     * @note 3. 子领地与其它家族成员的距离不能小于最小间距要求。
     */
    LDNDAPI static ValidateResult isSubLandPositionLegal(SharedLand const& land, LandAABB const& subRange);
};


} // namespace land