#include "LandCreateValidator.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/service/LandHierarchyService.h"

#include "ll/api/Expected.h"
#include "ll/api/service/Bedrock.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionHeightRange.h"

#include <memory>

using ll::operator""_trl;

namespace land {


ll::Expected<>
LandCreateValidator::validateCreateOrdinaryLand(LandRegistry& registry, Player& player, std::shared_ptr<Land> land) {
    auto locale = player.getLocaleCode();
    if (auto res = ensurePlayerLandCountNotExceeded(registry, player.getUuid(), locale); !res) {
        return res;
    }
    if (auto res = ensureLandRangeIsLegal(land->getAABB(), land->getDimensionId(), land->is3D(), locale); !res) {
        return res;
    }
    if (auto res = ensureLandNotInForbiddenRange(land->getAABB(), land->getDimensionId(), locale); !res) {
        return res;
    }
    if (auto res = ensureNoLandRangeConflict(registry, land, std::nullopt, locale); !res) {
        return res;
    }
    if (!land->isLeased()) { // 非租赁领地，确保领地范围不在仅租赁范围内
        if (auto res = ensureLandNotInLeaseOnlyRange(land->getAABB(), land->getDimensionId(), locale); !res) {
            return res;
        }
    }
    return {};
}

ll::Expected<> LandCreateValidator::validateChangeLandRange(
    LandRegistry&         registry,
    std::shared_ptr<Land> land,
    LandAABB              newRange,
    std::string_view      locale
) {
    if (auto res = ensureLandRangeIsLegal(newRange, land->getDimensionId(), land->is3D(), locale); !res) {
        return res;
    }
    if (auto res = ensureLandNotInForbiddenRange(newRange, land->getDimensionId(), locale); !res) {
        return res;
    }
    if (auto res = ensureNoLandRangeConflict(registry, land, newRange, locale); !res) {
        return res;
    }
    if (!land->isLeased()) { // 非租赁领地，确保新范围不在仅租赁范围内
        if (auto res = ensureLandNotInLeaseOnlyRange(newRange, land->getDimensionId(), locale); !res) {
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
    auto locale = player.getLocaleCode();
    if (auto res = ensurePlayerLandCountNotExceeded(registry, player.getUuid(), locale); !res) {
        return res;
    }
    if (auto res = ensureLandRangeIsLegal(subRange, land->getDimensionId(), true, locale); !res) {
        return res;
    }
    if (auto res = ensureSubLandPositionIsLegal(service, land, subRange, locale); !res) {
        return res;
    }
    return {};
}


ll::Expected<> LandCreateValidator::ensurePlayerLandCountNotExceeded(
    LandRegistry&    registry,
    mce::UUID const& uuids,
    std::string_view locale
) {
    auto count = static_cast<int>(registry.getLands(uuids).size());

    auto const& conf = ConfigProvider::getConstraintsConfig();

    if (!registry.isOperator(uuids) && count >= conf.maxLandsPerPlayer) {
        return ll::makeStringError(
            "领地数量超过上限, 当前领地数量: {0}, 最大领地数量: {1}"_trl(locale, count, conf.maxLandsPerPlayer)
        );
    }
    return {};
}

ll::Expected<>
LandCreateValidator::ensureLandNotInForbiddenRange(LandAABB const& range, LandDimid dimid, std::string_view locale) {
    auto const& conf = ConfigProvider::getConstraintsConfig();
    for (auto const& forbiddenRange : conf.forbiddenRanges) {
        if (forbiddenRange.dimensionId == dimid && LandAABB::isCollision(forbiddenRange.aabb, range)) {
            return ll::makeStringError(
                "领地范围在禁止区域内，当前范围: {0}, 禁止区域: {1}"_trl(
                    locale,
                    range.toString(),
                    forbiddenRange.aabb.toString()
                )
            );
        }
    }
    return {};
}

ll::Expected<>
LandCreateValidator::ensureLandNotInLeaseOnlyRange(LandAABB const& range, LandDimid dimid, std::string_view locale) {
    auto const& conf = ConfigProvider::getConstraintsConfig();
    for (auto const& leaseOnlyRange : conf.leaseOnlyRanges) {
        if (leaseOnlyRange.dimensionId == dimid && LandAABB::isCollision(leaseOnlyRange.aabb, range)) {
            return ll::makeStringError(
                "领地范围在仅租赁区域内，当前范围: {0}, 租赁区域: {1}"_trl(
                    locale,
                    range.toString(),
                    leaseOnlyRange.aabb.toString()
                )
            );
        }
    }
    return {};
}

ll::Expected<> LandCreateValidator::ensureLandRangeIsLegal(
    LandAABB const&  range,
    LandDimid        dimid,
    bool             is3D,
    std::string_view locale
) {
    auto const& conf = ConfigProvider::getConstraintsConfig().size;

    auto const length = range.getBlockCountX();
    auto const width  = range.getBlockCountZ();
    auto const height = range.getBlockCountY();

    auto dimension = ll::service::getLevel()->getDimension(dimid).lock();
    if (!dimension) {
        return ll::makeStringError("Failed to get dimension");
    }

    if (length < conf.minSideLength || width < conf.minSideLength) {
        return ll::makeStringError("领地范围过小，最小范围: {0}"_trl(locale, conf.minSideLength));
    }
    if (length > conf.maxSideLength || width > conf.maxSideLength) {
        return ll::makeStringError("领地范围过大，最大范围: {0}"_trl(locale, conf.maxSideLength));
    }

    if (is3D) {
        auto& dimHeightRange = dimension->mHeightRange;
        if (range.min.y < dimHeightRange->mMin) {
            return ll::makeStringError(
                "领地过高(维度高度)，当前高度: {0}, 最大高度: {1}(min/max)"_trl(
                    locale,
                    range.min.y,
                    dimHeightRange->mMin
                )
            );
        }
        if (range.max.y > dimHeightRange->mMax) {
            return ll::makeStringError(
                "领地过高(维度高度)，当前高度: {0}, 最大高度: {1}(min/max)"_trl(
                    locale,
                    range.max.y,
                    dimHeightRange->mMax
                )
            );
        }
        if (height < conf.minHeight) {
            return ll::makeStringError("领地高度过低 {0}<{1}"_trl(locale, height, conf.minHeight));
        }
    }

    return {};
}


ll::Expected<> LandCreateValidator::ensureNoLandRangeConflict(
    LandRegistry&                registry,
    std::shared_ptr<Land> const& land,
    std::optional<LandAABB>      newRange,
    std::string_view             locale
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
            return ll::makeStringError(
                "当前领地范围与领地 {0}({1}) 重叠，请调整领地范围!"_trl(locale, ld->getName(), ld->getId())
            );
        }
        if (!LandAABB::isComplisWithMinSpacing(ld->getAABB(), aabb, conf.minDistance)) {
            int actualDist = LandAABB::getMinSpacing(ld->getAABB(), aabb, conf.includeY);
            return ll::makeStringError(
                "当前领地范围与领地 {0}({1}) 间距过小，请调整领地范围\n当前间距: {2}, 最小间距: {3}"_trl(
                    locale,
                    ld->getName(),
                    ld->getId(),
                    actualDist,
                    conf.minDistance
                )
            );
        }
    }
    return {};
}

ll::Expected<> LandCreateValidator::ensureSubLandPositionIsLegal(
    service::LandHierarchyService& hierarchyService,
    std::shared_ptr<Land> const&   land,
    LandAABB const&                subRange,
    std::string_view               locale
) {
    if (!LandAABB::isContain(land->getAABB(), subRange)) {
        return ll::makeStringError(
            "子领地范围不在父领地范围内，当前范围: {0}, 父领地范围: {1}"_trl(
                locale,
                subRange.toString(),
                land->getAABB().toString()
            )
        );
    }

    auto const& conf = ConfigProvider::getSubLandConfig();

    auto const minSpacing = conf.minSpacing;
    bool const includeY   = conf.minSpacingIncludeY;

    auto family  = hierarchyService.getFamilyTree(land);
    auto parents = hierarchyService.getSelfAndAncestors(land);

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
            return ll::makeStringError(
                "当前领地范围与领地 {0}({1}) 重叠，请调整领地范围!"_trl(locale, member->getName(), member->getId())
            );
        }
        if (!LandAABB::isComplisWithMinSpacing(memberAABB, subRange, minSpacing, includeY)) {
            int actualDist = LandAABB::getMinSpacing(memberAABB, subRange, includeY);
            return ll::makeStringError(
                "当前领地范围与领地 {0}({1}) 间距过小，请调整领地范围\n当前间距: {2}, 最小间距: {3}"_trl(
                    locale,
                    member->getName(),
                    member->getId(),
                    actualDist,
                    minSpacing
                )
            );
        }
    }
    return {};
}


} // namespace land
