#include "LandPriceService.h"

#include "LandHierarchyService.h"
#include "pland/economy/PriceCalculate.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/utils/TimeUtils.h"

namespace land {
namespace service {

struct LandPriceService::Impl {
    LandHierarchyService& mHierarchyService;
};

LandPriceService::LandPriceService(LandHierarchyService& service) : impl(std::make_unique<Impl>(service)) {}
LandPriceService::~LandPriceService() = default;

int64_t LandPriceService::calculatePriceRecursively(
    std::shared_ptr<Land>                                              land,
    std::function<bool(std::shared_ptr<Land> const&, int64_t&)> const& func
) const {
    int64_t result = 0;

    auto iter = impl->mHierarchyService.getDescendants(land);
    for (auto i : iter) {
        if (!func) {
            result += i->getOriginalBuyPrice();
        } else {
            if (!func(i, result)) {
                break;
            }
        }
    }
    return result;
}

ll::Expected<LandPriceService::PriceResult>
LandPriceService::getOrdinaryLandPrice(LandAABB const& range, int dimId, bool is3D) const {
    return executeFormula(range, dimId, ConfigProvider::getPurchasePriceFormula(is3D));
}

ll::Expected<LandPriceService::PriceResult> LandPriceService::getSubLandPrice(LandAABB const& range, int dimId) const {
    return executeFormula(range, dimId, ConfigProvider::getSubLandPriceFormula());
}

int64_t LandPriceService::getRefundAmount(std::shared_ptr<Land> const& land) const {
    auto const& conf = ConfigProvider::getBoughtConfig();
    return PriceCalculate::calculateRefundsPrice(land->getOriginalBuyPrice(), conf.refundRate);
}

int64_t LandPriceService::getRefundAmountRecursively(std::shared_ptr<Land> const& land) const {
    auto const& conf = ConfigProvider::getBoughtConfig();
    return calculatePriceRecursively(land, [&conf](std::shared_ptr<Land> const& land, int64_t& price) {
        price += PriceCalculate::calculateRefundsPrice(land->getOriginalBuyPrice(), conf.refundRate);
        return true;
    });
}

ll::Expected<long long> LandPriceService::calculateDailyRent(LandAABB const& range, int dimId, bool is3D) const {
    if (auto expected = ensureEconomySystemAvailable(); !expected) {
        return ll::makeStringError(expected.error().message());
    }

    auto& formula = ConfigProvider::getLeasingFormula(is3D);

    auto result = executeFormula(range, dimId, formula);
    if (!result) {
        return result.error();
    }
    return std::llround(result.value().mDiscountedPrice);
}

ll::Expected<long long>
LandPriceService::calculateLeaseCost(LandAABB const& range, int dimId, bool is3D, int days) const {
    auto daily = calculateDailyRent(range, dimId, is3D);
    if (!daily) return daily.error();
    return daily.value() * static_cast<long long>(days);
}

LandPriceService::LeaseCostDetail
LandPriceService::calculateRenewCost(std::shared_ptr<Land> const& land, int days) const {
    LeaseCostDetail detail{};
    if (!land || !land->isLeased()) return detail;

    auto dailyRent = 0;
    if (auto calc = calculateDailyRent(land->getAABB(), land->getDimensionId(), land->is3D())) {
        dailyRent = calc.value();
    }

    auto now = time_utils::nowSeconds();

    // 计算滞纳天数，当前时间与租约结束时间的差值向上取整
    auto overdueDays = time_utils::ceilDays(now - land->getLeaseEndAt());
    if (overdueDays < 0) overdueDays = 0;

    detail.dailyRent   = dailyRent;   // 日租金
    detail.overdueDays = overdueDays; // 滞纳天数
    // 基础金额 = 日租金 * (滞纳天数 + 续租天数)，续租天数不能小于0
    detail.baseAmount = dailyRent * (overdueDays + std::max(0, days));

    auto rate      = ConfigProvider::getLeasingConfig().freeze.penaltyRatePerDay;
    detail.penalty = static_cast<long long>(std::ceil(dailyRent * overdueDays * rate)); // 计算滞纳金，向上取整
    detail.total   = detail.baseAmount + detail.penalty; // 计算总金额 = 基础金额 + 滞纳金
    return detail;
}

ll::Expected<> LandPriceService::ensureEconomySystemAvailable() {
    if (!ConfigProvider::isEconomySystemEnabled()) {
        return ll::makeStringError("Economy system is not enabled");
    }
    return {};
}

ll::Expected<LandPriceService::PriceResult>
LandPriceService::executeFormula(LandAABB const& range, int dimId, std::string const& formula) const {
    if (auto expected = ensureEconomySystemAvailable(); !expected) {
        return ll::makeStringError(expected.error().message());
    }
    auto variable = PriceCalculate::Variable::make(range, dimId);
    auto expected = PriceCalculate::eval(formula, variable);
    if (!expected) return ll::makeStringError(expected.error().message());

    auto original    = expected.value();
    auto multipliers = ConfigProvider::getDimensionPriceMultiplier(dimId);
    if (multipliers) {
        original *= *multipliers;
    }

    auto discountedPrice = PriceCalculate::calculateDiscountPrice(original, ConfigProvider::getDiscountRate());
    return PriceResult{static_cast<long long>(original), discountedPrice, multipliers};
}


} // namespace service
} // namespace land