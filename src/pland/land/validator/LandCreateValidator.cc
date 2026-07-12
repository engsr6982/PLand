#include "LandCreateValidator.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/service/LandHierarchyService.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/McUtils.h"

#include "ll/api/Expected.h"
#include "ll/api/service/Bedrock.h"

#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionHeightRange.h"

#include "nonstd/expected.hpp"

#include <magic_enum.hpp>
#include <memory>
#include <stdexcept>
#include <utility>


namespace land {


ll::Expected<>
LandCreateValidator::validateCreateOrdinaryLand(LandRegistry& registry, Player& player, std::shared_ptr<Land> land) {
    if (auto res = ensurePlayerLandCountNotExceeded(registry, player.getUuid()); !res) {
        return res;
    }
    if (auto res = ensureLandRangeIsLegal(land->getAABB(), land->getDimensionId(), land->is3D()); !res) {
        return res;
    }
    if (auto res = ensureLandNotInForbiddenRange(land->getAABB(), land->getDimensionId()); !res) {
        return res;
    }
    if (auto res = ensureNoLandRangeConflict(registry, land); !res) {
        return res;
    }
    if (!land->isLeased()) { // 非租赁领地，确保领地范围不在仅租赁范围内
        if (auto res = ensureLandNotInLeaseOnlyRange(land->getAABB(), land->getDimensionId()); !res) {
            return res;
        }
    }
    return {};
}

ll::Expected<>
LandCreateValidator::validateChangeLandRange(LandRegistry& registry, std::shared_ptr<Land> land, LandAABB newRange) {
    if (auto res = ensureLandRangeIsLegal(newRange, land->getDimensionId(), land->is3D()); !res) {
        return res;
    }
    if (auto res = ensureLandNotInForbiddenRange(newRange, land->getDimensionId()); !res) {
        return res;
    }
    if (auto res = ensureNoLandRangeConflict(registry, land, newRange); !res) {
        return res;
    }
    if (!land->isLeased()) { // 非租赁领地，确保新范围不在仅租赁范围内
        if (auto res = ensureLandNotInLeaseOnlyRange(newRange, land->getDimensionId()); !res) {
            return res;
        }
    }
    return {};
}

ll::Expected<> LandCreateValidator::validateCreateSubLand(
    Player&                        player,
    std::shared_ptr<Land>          land,
    LandAABB const&                subRange,
    LandRegistry&                  registry,
    service::LandHierarchyService& service
) {
    if (auto res = ensurePlayerLandCountNotExceeded(registry, player.getUuid()); !res) {
        return res;
    }
    if (auto res = ensureLandRangeIsLegal(subRange, land->getDimensionId(), true); !res) {
        return res;
    }
    if (auto res = ensureSubLandPositionIsLegal(service, land, subRange); !res) {
        return res;
    }
    return {};
}


ll::Expected<> LandCreateValidator::ensurePlayerLandCountNotExceeded(LandRegistry& registry, mce::UUID const& uuids) {
    auto count = static_cast<int>(registry.getLands(uuids).size());

    auto const& conf = ConfigProvider::getConstraintsConfig();

    // 非管理员 && 领地数量超过限制
    if (!registry.isOperator(uuids) && count >= conf.maxLandsPerPlayer) {
        return makeError<LandCountExceededContext>(count, conf.maxLandsPerPlayer);
    }
    return {};
}

ll::Expected<> LandCreateValidator::ensureLandNotInForbiddenRange(LandAABB const& range, LandDimid dimid) {
    auto const& conf = ConfigProvider::getConstraintsConfig();
    for (auto const& forbiddenRange : conf.forbiddenRanges) {
        if (forbiddenRange.dimensionId == dimid && LandAABB::isCollision(forbiddenRange.aabb, range)) {
            return makeError<LandInForbiddenRangeContext>(range, forbiddenRange.aabb);
        }
    }
    return {};
}

ll::Expected<> LandCreateValidator::ensureLandNotInLeaseOnlyRange(LandAABB const& range, LandDimid dimid) {
    auto const& conf = ConfigProvider::getConstraintsConfig();
    for (auto const& leaseOnlyRange : conf.leaseOnlyRanges) {
        if (leaseOnlyRange.dimensionId == dimid && LandAABB::isCollision(leaseOnlyRange.aabb, range)) {
            return makeError<LandInLeaseOnlyRangeContext>(range, leaseOnlyRange.aabb);
        }
    }
    return {};
}

ll::Expected<> LandCreateValidator::ensureLandRangeIsLegal(LandAABB const& range, LandDimid dimid, bool is3D) {
    auto const& conf = ConfigProvider::getConstraintsConfig().size;

    auto const length = range.getBlockCountX();
    auto const width  = range.getBlockCountZ();
    auto const height = range.getBlockCountY();

    auto dimension = ll::service::getLevel()->getDimension(dimid).lock();
    if (!dimension) {
        return ll::makeStringError("Failed to get dimension");
    }

    // 范围长度和宽度
    if (length < conf.minSideLength || width < conf.minSideLength) {
        return makeError<LandRangeErrorContext>(ErrorCode::LandRangeTooSmall, conf.minSideLength);
    }
    if (length > conf.maxSideLength || width > conf.maxSideLength) {
        return makeError<LandRangeErrorContext>(ErrorCode::LandRangeTooLarge, conf.maxSideLength);
    }

    if (is3D) {
        // 校验维度范围
        auto& dimHeightRange = dimension->mHeightRange;
        if (range.min.y < dimHeightRange->mMin) {
            return makeError<LandHeightErrorContext>(
                ErrorCode::LandOutOfDimensionHeightRange,
                range.min.y,
                dimHeightRange->mMin
            );
        }
        if (range.max.y > dimHeightRange->mMax) {
            return makeError<LandHeightErrorContext>(
                ErrorCode::LandOutOfDimensionHeightRange,
                range.max.y,
                dimHeightRange->mMax
            );
        }
        // 校验业务允许范围
        if (height < conf.minHeight) {
            return makeError<LandHeightErrorContext>(ErrorCode::LandHeightTooSmall, height, conf.minHeight);
        }
    }

    return {};
}


ll::Expected<> LandCreateValidator::ensureNoLandRangeConflict(
    LandRegistry&                registry,
    std::shared_ptr<Land> const& land,
    std::optional<LandAABB>      newRange
) {
    auto& aabb = newRange ? *newRange : land->getAABB();

    auto const& conf = ConfigProvider::getConstraintsConfig().spacing;

    auto expanded = aabb.expanded(conf.minDistance, conf.includeY);
    auto lands    = registry.getLandAt(expanded.min.as(), expanded.max.as(), land->getDimensionId());
    if (lands.empty()) {
        return {};
    }

    for (auto& ld : lands) {
        if (newRange && ld == land) {
            continue; // 仅在更改范围时排除自己
        }

        if (LandAABB::isCollision(ld->getAABB(), aabb)) {
            // 领地范围与其他领地冲突
            return makeError<LandRangeConflictContext>(aabb, ld);
        }
        if (!LandAABB::isComplisWithMinSpacing(ld->getAABB(), aabb, conf.minDistance)) {
            // 领地范围与其他领地间距过小
            int actualDist = LandAABB::getMinSpacing(ld->getAABB(), aabb, conf.includeY);
            return makeError<LandSpacingContext>(actualDist, conf.minDistance, ld);
        }
    }
    return {};
}

ll::Expected<> LandCreateValidator::ensureSubLandPositionIsLegal(
    service::LandHierarchyService& hierarchyService,
    std::shared_ptr<Land> const&   land,
    LandAABB const&                subRange
) {
    // 子领地必须位于父领地内
    if (!LandAABB::isContain(land->getAABB(), subRange)) {
        return makeError<SubLandNotInParentContext>(land, subRange);
    }


    auto const& conf = ConfigProvider::getSubLandConfig();

    auto const minSpacing = conf.minSpacing;
    bool const includeY   = conf.minSpacingIncludeY;

    auto family  = hierarchyService.getFamilyTree(land);       // 整个领地家族
    auto parents = hierarchyService.getSelfAndAncestors(land); // 相对于 land 的所有父领地

    // 子领地不能与家族内其他领地冲突
    for (auto& member : family) {
        if (member == land) {
            continue; // 排除自身(因为 sub 是 land 的子领地，所以 land 必然与 sub 冲突)
        }
        if (parents.contains(member)) {
            continue; // 排除父领地(因为 sub 是 land 的子领地，那么必然与整个家族内的父领地冲突)
        }

        auto& memberAABB = member->getAABB();

        if (LandAABB::isCollision(memberAABB, subRange)) {
            // 子领地与家族内其他领地冲突
            return makeError<LandRangeConflictContext>(subRange, member);
        }
        if (!LandAABB::isComplisWithMinSpacing(memberAABB, subRange, minSpacing, includeY)) {
            // 子领地与家族内其他领地间距过小
            int actualDist = LandAABB::getMinSpacing(memberAABB, subRange, includeY);
            return makeError<LandSpacingContext>(actualDist, minSpacing, member);
        }
    }
    return {};
}


std::string LandCreateValidator::ValidateError::message(std::string_view locale) const noexcept {
    if (!locale.empty()) {
        return translateError(std::string{locale});
    }
    return "Un translated ValidateError";
}

void LandCreateValidator::ValidateError::sendTo(Player& player) const {
    feedback_utils::sendErrorText(player, translateError(player.getLocaleCode()));
}

using ll::operator""_trl;
std::string LandCreateValidator::ValidateError::translateError(std::string const& localeCode) const {
    if (this->mContext) {
        return mContext->translateError(localeCode);
    }
    return "翻译失败，没有有效的异常上下文"_trl(localeCode);
}


LandCreateValidator::LandCountExceededContext::LandCountExceededContext(int count, int maxCount)
: ErrorContext(ErrorCode::LandCountLimitExceeded),
  count(count),
  maxCount(maxCount) {}
std::string LandCreateValidator::LandCountExceededContext::translateError(std::string const& localeCode) const {
    return "领地数量超过上限, 当前领地数量: {0}, 最大领地数量: {1}"_trl(localeCode, count, maxCount);
}


LandCreateValidator::LandInForbiddenRangeContext::LandInForbiddenRangeContext(
    LandAABB const& range,
    LandAABB const& forbiddenRange
)
: ErrorContext(ErrorCode::LandInForbiddenRange),
  range(range),
  forbiddenRange(forbiddenRange) {}
std::string LandCreateValidator::LandInForbiddenRangeContext::translateError(std::string const& localeCode) const {
    return "领地范围在禁止区域内，当前范围: {0}, 禁止区域: {1}"_trl(
        localeCode,
        range.toString(),
        forbiddenRange.toString()
    );
}


LandCreateValidator::LandInLeaseOnlyRangeContext::LandInLeaseOnlyRangeContext(
    LandAABB const& range,
    LandAABB const& leaseOnlyRange
)
: ErrorContext(ErrorCode::LandInLeaseOnlyRange),
  range(range),
  leaseOnlyRange(leaseOnlyRange) {}

std::string LandCreateValidator::LandInLeaseOnlyRangeContext::translateError(std::string const& localeCode) const {
    return "领地范围在仅租赁区域内，当前范围: {0}, 租赁区域: {1}"_trl(
        localeCode,
        range.toString(),
        leaseOnlyRange.toString()
    );
}


LandCreateValidator::LandRangeErrorContext::LandRangeErrorContext(ErrorCode code, int limit)
: ErrorContext(code),
  limitSize(limit) {}
std::string LandCreateValidator::LandRangeErrorContext::translateError(std::string const& localeCode) const {
    if (code == ErrorCode::LandRangeTooSmall) {
        return "领地范围过小，最小范围: {0}"_trl(localeCode, this->limitSize);
    } else if (code == ErrorCode::LandRangeTooLarge) {
        return "领地范围过大，最大范围: {0}"_trl(localeCode, this->limitSize);
    } else {
        throw std::runtime_error("Unknow ErrorCode in LandRangeErrorContext");
    }
}


LandCreateValidator::LandHeightErrorContext::LandHeightErrorContext(ErrorCode code, int actual, int limit)
: ErrorContext(code),
  actualHeight(actual),
  limitHeight(limit) {}
std::string LandCreateValidator::LandHeightErrorContext::translateError(std::string const& localeCode) const {
    if (code == ErrorCode::LandOutOfDimensionHeightRange) {
        return "领地过高(维度高度)，当前高度: {0}, 最大高度: {1}(min/max)"_trl(localeCode, actualHeight, limitHeight);
    } else if (code == ErrorCode::LandHeightTooSmall) {
        return "领地高度过低 {0}<{1}"_trl(localeCode, this->actualHeight, this->limitHeight);
    } else {
        throw std::runtime_error("Unknow ErrorCode in LandHeightErrorContext");
    }
}


LandCreateValidator::LandRangeConflictContext::LandRangeConflictContext(
    LandAABB const&       range,
    std::shared_ptr<Land> conflictLand
)
: ErrorContext(ErrorCode::LandRangeConflict),
  range(range),
  conflictLand(std::move(conflictLand)) {}
std::string LandCreateValidator::LandRangeConflictContext::translateError(std::string const& localeCode) const {
    return "当前领地范围与领地 {0}({1}) 重叠，请调整领地范围!"_trl(
        localeCode,
        conflictLand->getName(),
        conflictLand->getId()
    );
}


LandCreateValidator::LandSpacingContext::LandSpacingContext(
    int                   spacing,
    int                   minSpacing,
    std::shared_ptr<Land> conflictLand
)
: ErrorContext(ErrorCode::LandSpacingTooSmall),
  spacing(spacing),
  minSpacing(minSpacing),
  conflictLand(std::move(conflictLand)) {}
std::string LandCreateValidator::LandSpacingContext::translateError(std::string const& localeCode) const {
    return "当前领地范围与领地 {0}({1}) 间距过小，请调整领地范围\n当前间距: {2}, 最小间距: {3}"_trl(
        localeCode,
        conflictLand->getName(),
        conflictLand->getId(),
        spacing,
        minSpacing
    );
}


LandCreateValidator::SubLandNotInParentContext::SubLandNotInParentContext(
    std::shared_ptr<Land> parent,
    LandAABB const&       subRange
)
: ErrorContext(ErrorCode::SubLandOutOfParentLandRange),
  parent(std::move(parent)),
  subRange(subRange) {}
std::string LandCreateValidator::SubLandNotInParentContext::translateError(std::string const& localeCode) const {
    return "子领地范围不在父领地范围内，当前范围: {0}, 父领地范围: {1}"_trl(
        localeCode,
        subRange.toString(),
        parent->getAABB().toString()
    );
}


} // namespace land
