#include "LandCreateValidator.h"
#include "ll/api/service/Bedrock.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionHeightRange.h"
#include "pland/aabb/LandAABB.h"
#include "pland/infra/Config.h"
#include "pland/land/LandRegistry.h"
#include "pland/mod/ModEntry.h"
#include "pland/utils/McUtils.h"

namespace land {


LandCreateValidator::ValidateResult LandCreateValidator::validateCreateOrdinaryLand(Player& player, SharedLand land) {
    if (auto res = isPlayerLandCountLimitExceeded(player.getUuid().asString()); !res) {
        return res;
    }
    if (auto res = isLandRangeLegal(land->getAABB(), land->getDimensionId(), land->is3D()); !res) {
        return res;
    }
    if (auto res = isLandInForbiddenRange(land->getAABB(), land->getDimensionId()); !res) {
        return res;
    }
    if (auto res = isLandRangeWithOtherCollision(land); !res) {
        return res;
    }
    return {};
}

LandCreateValidator::ValidateResult LandCreateValidator::validateChangeLandRange(SharedLand land, LandAABB newRange) {
    if (auto res = isLandRangeLegal(newRange, land->getDimensionId(), land->is3D()); !res) {
        return res;
    }
    if (auto res = isLandInForbiddenRange(newRange, land->getDimensionId()); !res) {
        return res;
    }
    if (auto res = isLandRangeWithOtherCollision(land, newRange); !res) {
        return res;
    }
    return {};
}

LandCreateValidator::ValidateResult
LandCreateValidator::validateCreateSubLand(Player& player, SharedLand land, LandAABB const& subRange) {
    if (auto res = isPlayerLandCountLimitExceeded(player.getUuid().asString()); !res) {
        return res;
    }
    if (auto res = isLandRangeLegal(subRange, land->getDimensionId(), true); !res) {
        return res;
    }
    if (auto res = isSubLandPositionLegal(land, subRange); !res) {
        return res;
    }
    return {};
}


LandCreateValidator::ValidateResult LandCreateValidator::isPlayerLandCountLimitExceeded(UUIDs const& uuids) {
    auto& registry = LandRegistry::getInstance();
    auto  count    = static_cast<int>(registry.getLands(uuids).size());
    auto& maxCount = Config::cfg.land.maxLand;

    // 非管理员 && 领地数量超过限制
    if (!registry.isOperator(uuids) && count >= Config::cfg.land.maxLand) {
        return std::unexpected(ErrorContext::landCountLimitExceeded(count, maxCount));
    }

    return {};
}

LandCreateValidator::ValidateResult
LandCreateValidator::isLandInForbiddenRange(LandAABB const& range, LandDimid dimid) {
    for (auto const& forbiddenRange : Config::cfg.land.bought.forbiddenRanges) {
        if (forbiddenRange.dimensionId == dimid && LandAABB::isCollision(forbiddenRange.aabb, range)) {
            return std::unexpected(ErrorContext::landInForbiddenRange(range, forbiddenRange.aabb));
        }
    }
    return {};
}

LandCreateValidator::ValidateResult
LandCreateValidator::isLandRangeLegal(LandAABB const& range, LandDimid dimid, bool is3D) {
    auto const& squareRange = Config::cfg.land.bought.squareRange;

    auto const length = range.getDepth();
    auto const width  = range.getWidth();
    auto const height = range.getHeight();

    auto dimension = ll::service::getLevel()->getDimension(dimid).lock();
    if (!dimension) {
        mod::ModEntry::getInstance().getSelf().getLogger().warn(
            "LandCreateValidator::isLandRangeLegal: dimension {} is not found",
            dimid
        );
        return std::unexpected(ErrorContext{
            .code = ErrorCode::Undefined,
        });
    }

    if (length < squareRange.min || width < squareRange.min) {
        return std::unexpected(ErrorContext::landRangeTooSmall(range, squareRange.min));
    }

    if (length > squareRange.max || width > squareRange.max) {
        return std::unexpected(ErrorContext::landRangeTooLarge(range, squareRange.max));
    }

    if (is3D && height < squareRange.minHeight) {
        return std::unexpected(ErrorContext::landHeightTooSmall(range, squareRange.minHeight));
    }

    if (is3D && height > dimension->mHeightRange->mMax) {
        return std::unexpected(ErrorContext::landHeightTooLarge(range, dimension->mHeightRange->mMax));
    }

    return {};
}

LandCreateValidator::ValidateResult
LandCreateValidator::isLandRangeWithOtherCollision(SharedLand const& land, std::optional<LandAABB> newRange) {
    return isLandRangeWithOtherCollision(&LandRegistry::getInstance(), land, newRange);
}

LandCreateValidator::ValidateResult LandCreateValidator::isLandRangeWithOtherCollision(
    LandRegistry*           registry,
    SharedLand const&       land,
    std::optional<LandAABB> newRange
) {
    auto& aabb  = newRange ? *newRange : land->getAABB();
    auto  lands = registry->getLandAt(aabb.min.as(), aabb.max.as(), land->getDimensionId());
    if (lands.empty()) {
        return {};
    }

    for (auto& member : lands) {
        if (newRange && member == land) {
            continue; // 仅在更改范围时排除自己
        }

        if (LandAABB::isCollision(member->getAABB(), aabb)) {
            // 领地范围与其他领地冲突
            return std::unexpected(ErrorContext::landRangeWithOtherCollision(member));
        }
        if (!LandAABB::isComplisWithMinSpacing(member->getAABB(), aabb, Config::cfg.land.minSpacing)) {
            // 领地范围与其他领地间距过小
            return std::unexpected(ErrorContext::landSpacingTooSmall(
                member,
                LandAABB::getMinSpacing(member->getAABB(), aabb),
                Config::cfg.land.minSpacing
            ));
        }
    }
    return {};
}

LandCreateValidator::ValidateResult
LandCreateValidator::isSubLandPositionLegal(SharedLand const& land, LandAABB const& subRange) {
    // 子领地必须位于父领地内
    if (!LandAABB::isContain(land->getAABB(), subRange)) {
        return std::unexpected(ErrorContext::subLandNotInParent(land, subRange));
    }

    auto family  = land->getFamilyTree();       // 整个领地家族
    auto parents = land->getSelfAndAncestors(); // 相对于 land 的所有父领地

    // 子领地不能与家族内其他领地冲突
    for (auto& member : family) {
        if (member == land) {
            continue; // 排除自身(因为 sub 是 land 的子领地，所以 land 必然与 sub 冲突)
        }
        if (parents.contains(member)) {
            continue; // 排除父领地(因为 sub 是 land 的子领地，那么必然与整个家族内的父领地冲突)
        }

        if (LandAABB::isCollision(member->getAABB(), subRange)) {
            // 子领地与家族内其他领地冲突
            return std::unexpected(ErrorContext::landRangeWithOtherCollision(member));
        }
        if (!LandAABB::isComplisWithMinSpacing(member->getAABB(), subRange, Config::cfg.land.subLand.minSpacing)) {
            // 子领地与家族内其他领地间距过小
            return std::unexpected(ErrorContext::landSpacingTooSmall(
                member,
                LandAABB::getMinSpacing(member->getAABB(), subRange),
                Config::cfg.land.minSpacing
            ));
        }
    }
    return {};
}


// ErrorContext
LandCreateValidator::ErrorContext LandCreateValidator::ErrorContext::undefined() { return {ErrorCode::Undefined}; }

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landCountLimitExceeded(int currentCount, int maxCount) {
    return {
        .code         = ErrorCode::LandCountLimitExceeded,
        .currentCount = currentCount,
        .maxCount     = maxCount,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landInForbiddenRange(LandAABB const& range, LandAABB const& forbiddenRange) {
    return {
        .code           = ErrorCode::LandInForbiddenRange,
        .currentRange   = range,
        .forbiddenRange = forbiddenRange,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landRangeTooSmall(LandAABB const& range, int minRange) {
    return {
        .code         = ErrorCode::LandRangeTooSmall,
        .currentRange = range,
        .minRange     = minRange,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landRangeTooLarge(LandAABB const& range, int maxRange) {
    return {
        .code         = ErrorCode::LandRangeTooLarge,
        .currentRange = range,
        .maxRange     = maxRange,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landHeightTooSmall(LandAABB const& range, int minHeight) {
    return {
        .code         = ErrorCode::LandHeightTooSmall,
        .currentRange = range,
        .minHeight    = minHeight,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landHeightTooLarge(LandAABB const& range, int maxHeight) {
    return {
        .code         = ErrorCode::LandHeightTooLarge,
        .currentRange = range,
        .maxHeight    = maxHeight,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landRangeWithOtherCollision(SharedLand const& conflictLand) {
    return {
        .code         = ErrorCode::LandRangeWithOtherCollision,
        .conflictLand = conflictLand,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::landSpacingTooSmall(SharedLand const& conflictLand, int spacing, int minSpacing) {
    return {
        .code         = ErrorCode::LandSpacingTooSmall,
        .conflictLand = conflictLand,
        .spacing      = spacing,
        .minSpacing   = minSpacing,
    };
}

LandCreateValidator::ErrorContext
LandCreateValidator::ErrorContext::subLandNotInParent(SharedLand const& land, LandAABB const& subRange) {
    return {
        .code         = ErrorCode::SubLandNotInParent,
        .currentRange = subRange,
        .conflictLand = land,
    };
}


void LandCreateValidator::sendErrorMessage(Player& player, ErrorContext const& ctx) {
    std::string msg;

    switch (ctx.code) {
    case ErrorCode::Undefined: {
        msg = "未定义错误"_trf(player);
        break;
    }
    case ErrorCode::LandCountLimitExceeded: {
        msg = "领地数量超过上限, 当前领地数量: {0}, 最大领地数量: {1}"_trf(player, ctx.currentCount, ctx.maxCount);
        break;
    }
    case ErrorCode::LandInForbiddenRange: {
        msg = "领地范围在禁止区域内，当前范围: {0}, 禁止区域: {1}"_trf(
            player,
            ctx.currentRange.toString(),
            ctx.forbiddenRange.toString()
        );
        break;
    }
    case ErrorCode::LandRangeTooSmall: {
        if (ctx.currentRange.getWidth() < ctx.minRange) {
            msg = "领地宽度过小(z轴)，当前宽度: {0}, 最小宽度: {1}"_trf(
                player,
                ctx.currentRange.getWidth(),
                ctx.minRange
            );
        } else {
            msg = "领地长度过小(x轴)，当前长度: {0}, 最小长度: {1}"_trf(
                player,
                ctx.currentRange.getDepth(),
                ctx.minRange
            );
        }
        break;
    }
    case ErrorCode::LandRangeTooLarge: {
        if (ctx.currentRange.getWidth() > ctx.maxRange) {
            msg = "领地宽度过大(z轴)，当前宽度: {0}, 最大宽度: {1}"_trf(
                player,
                ctx.currentRange.getWidth(),
                ctx.maxRange
            );
        } else {
            msg = "领地长度过大(x轴)，当前长度: {0}, 最大长度: {1}"_trf(
                player,
                ctx.currentRange.getDepth(),
                ctx.maxRange
            );
        }
        break;
    }
    case ErrorCode::LandHeightTooSmall: {
        msg = "领地高度过低，当前高度: {0}, 最小高度: {1}"_trf(player, ctx.currentRange.getHeight(), ctx.minHeight);
        break;
    }
    case ErrorCode::LandHeightTooLarge: {
        msg = "领地高度过高，当前高度: {0}, 最大高度: {1}"_trf(player, ctx.currentRange.getHeight(), ctx.maxHeight);
        break;
    }
    case ErrorCode::LandRangeWithOtherCollision: {
        msg = "当前领地范围与领地 {0}({1}) 重叠，请调整领地范围!"_trf(
            player,
            ctx.conflictLand->getName(), // 0
            ctx.conflictLand->getId()    // 1
        );
        break;
    }
    case ErrorCode::LandSpacingTooSmall: {
        msg = "当前领地范围与领地 {0}({1}) 间距过小，请调整领地范围\n当前间距: {2}, 最小间距: {3}"_trf(
            player,
            ctx.conflictLand->getName(), // 0
            ctx.conflictLand->getId(),   // 1
            ctx.spacing,                 // 2
            ctx.minSpacing               // 3
        );
        break;
    }
    case ErrorCode::SubLandNotInParent: {
        msg = "子领地范围不在父领地范围内，当前范围: {0}, 父领地范围: {1}"_trf(
            player,
            ctx.currentRange.toString(),
            ctx.conflictLand->getAABB().toString()
        );
        break;
    }
    }

    mc_utils::sendText<mc_utils::LogLevel::Error>(player, msg);
}


} // namespace land