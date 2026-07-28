#include "pland/gui/LandBuyGUI.h"

#include "mc/world/actor/player/Player.h"

#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/economy/EconomySystem.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/LandResizeSettlement.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/land/repo/StorageError.h"
#include "pland/selector/ABSelector.h"
#include "pland/selector/SelectorManager.h"
#include "pland/selector/land/LandResizeSelector.h"
#include "pland/selector/land/OrdinaryLandCreateSelector.h"
#include "pland/selector/land/SubLandCreateSelector.h"
#include "pland/service/LandManagementService.h"
#include "pland/service/LandPriceService.h"
#include "pland/service/LeasingService.h"
#include "pland/service/ServiceLocator.h"
#include "pland/utils/FeedbackUtils.h"
#include "utils/BackUtils.h"


#include <cassert>
#include <climits>
#include <ll/api/form/CustomForm.h>
#include <ll/api/form/SimpleForm.h>
#include <string>


namespace land::gui {

void LandBuyGUI::sendTo(Player& player) {
    auto localeCode = player.getLocaleCode();

    auto manager = land::PLand::getInstance().getSelectorManager();
    if (!manager->hasSelector(player)) {
        feedback_utils::sendErrorText(player, "请先使用 /pland new 来选择领地"_trl(localeCode));
        return;
    }

    auto abstractSelect = manager->getSelector(player);
    assert(abstractSelect != nullptr);

    auto selector = abstractSelect->as<ABSelector>();
    if (!selector) {
        // TODO: handle thirdparty AbstractSelector
        return;
    }

    if (!selector->isPointABSet()) {
        feedback_utils::sendErrorText(player, "您还没有选择领地范围，无法进行购买!"_trl(localeCode));
        return;
    }

    if (auto def = selector->as<OrdinaryLandCreateSelector>()) {
        _chooseHoldType(player, def);
    } else if (auto re = selector->as<LandResizeSelector>()) {
        _impl(player, re);
    } else if (auto sub = selector->as<SubLandCreateSelector>()) {
        _impl(player, sub);
    }
}
void LandBuyGUI::_chooseHoldType(Player& player, OrdinaryLandCreateSelector* def) {
    if (ConfigProvider::isLeasingModelEnabled() && ConfigProvider::isLeasingDimensionAllowed(def->getDimensionId())) {
        auto fm = ll::form::ModalForm{};

        auto localeCode = player.getLocaleCode();
        fm.setTitle("[PLand] | 购买/租赁"_trl(localeCode));
        fm.setContent("请选择领地购买模式"_trl(localeCode));
        fm.setUpperButton("永久买断"_trl(localeCode));
        fm.setLowerButton("租赁模式"_trl(localeCode));
        fm.sendTo(player, [def](Player& player, ll::form::ModalFormResult result, ll::form::FormCancelReason) {
            if (!result) {
                return;
            }
            if (static_cast<bool>(result.value())) _impl(player, def);
            else _chooseLeaseDays(player, def);
        });
        return;
    }
    _impl(player, def);
}

void LandBuyGUI::_impl(Player& player, OrdinaryLandCreateSelector* selector) {
    auto localeCode = player.getLocaleCode();

    bool const is3D  = selector->is3D();
    auto       range = selector->newLandAABB();
    range->fix();

    auto const volume = range->getVolume();
    if (volume >= INT_MAX) {
        feedback_utils::sendErrorText(player, "领地体积过大，无法购买"_trl(localeCode));
        return;
    }

    std::string content = "领地类型: {}\n体积: {}x{}x{} = {}\n范围: {}"_trl(
        localeCode,
        is3D ? "3D" : "2D",
        range->getBlockCountX(),
        range->getBlockCountZ(),
        range->getBlockCountY(),
        volume,
        range->toString()
    );

    std::optional<int64_t> discountedPrice;
    if (ConfigProvider::isEconomySystemEnabled()) {
        auto& service = PLand::getInstance().getServiceLocator().getLandPriceService();
        if (auto result = service.getOrdinaryLandPrice(*range, selector->getDimensionId(), is3D)) {
            discountedPrice  = result->mDiscountedPrice;
            content         += "\n原价: {}\n折扣价: {}\n{}"_trl(
                localeCode,
                result->mOriginalPrice,
                result->mDiscountedPrice,
                EconomySystem::getInstance().getCostMessage(player, result->mDiscountedPrice)
            );
        } else {
            feedback_utils::sendErrorText(player, "价格表达式解析失败，请联系管理员"_trl(localeCode));
            PLand::getInstance().getSelf().getLogger().error(result.error().message());
            return;
        }
    }

    auto fm = ll::form::SimpleForm{};
    fm.setTitle("[PLand] | 购买领地"_trl(localeCode));
    fm.setContent(content);
    fm.appendButton(
        "确认购买"_trl(localeCode),
        "textures/ui/realms_green_check",
        "path",
        [discountedPrice, selector](Player& pl) {
            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
            if (auto exp = service.buyLand(pl, selector, discountedPrice.value_or(0))) {
                feedback_utils::notifySuccess(pl, "购买领地成功"_trl(pl.getLocaleCode()));
            } else {
                feedback_utils::sendError(pl, exp.error());
            }
        }
    );
    fm.appendButton("暂存订单"_trl(localeCode), "textures/ui/recipe_book_icon", "path"); // close
    fm.appendButton("放弃订单"_trl(localeCode), "textures/ui/cancel", "path", [](Player& pl) {
        land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
    });

    fm.sendTo(player);
}

void LandBuyGUI::_chooseLeaseDays(Player& player, OrdinaryLandCreateSelector* selector) {
    auto localeCode = player.getLocaleCode();

    bool const is3D  = selector->is3D();
    auto       range = selector->newLandAABB();
    range->fix();

    auto const volume = range->getVolume();
    if (volume >= INT_MAX) {
        feedback_utils::sendErrorText(player, "领地体积过大，无法租赁"_trl(localeCode));
        return;
    }

    auto dailyExp = PLand::getInstance().getServiceLocator().getLandPriceService().calculateDailyRent(
        *range,
        selector->getDimensionId(),
        is3D
    );
    if (!dailyExp) {
        feedback_utils::sendErrorText(player, "价格表达式解析失败，请联系管理员"_trl(localeCode));
        PLand::getInstance().getSelf().getLogger().error(dailyExp.error().message());
        return;
    }
    auto dailyRent = dailyExp.value();

    auto const& duration = ConfigProvider::getLeasingConfig().duration;

    std::string baseInfo = "领地类型: {}\n体积: {}x{}x{} = {}\n范围: {}\n每日租金: {}"_trl(
        localeCode,
        is3D ? "3D" : "2D",
        range->getBlockCountX(),
        range->getBlockCountZ(),
        range->getBlockCountY(),
        volume,
        range->toString(),
        dailyRent
    );

    auto fm = ll::form::CustomForm{};
    fm.setTitle("[PLand] | 租赁领地"_trl(localeCode));
    fm.appendLabel(baseInfo);
    fm.appendSlider(
        "days",
        "租赁天数"_trl(localeCode),
        duration.minPeriod,
        duration.maxPeriod,
        1.0,
        duration.minPeriod
    );
    fm.setSubmitButton("下一步"_trl(localeCode));
    fm.sendTo(
        player,
        [selector, dailyRent, baseInfo](Player& pl, ll::form::CustomFormResult res, ll::form::FormCancelReason) {
            if (!res) {
                return;
            }

            auto days  = static_cast<int>(std::get<double>(res->at("days")));
            auto total = dailyRent * static_cast<long long>(days);
            _confirmLeaseDays(pl, selector, days, total, baseInfo);
        }
    );
}
void LandBuyGUI::_confirmLeaseDays(
    Player&                     player,
    OrdinaryLandCreateSelector* selector,
    int                         days,
    int64_t                     totalPrice,
    std::string                 baseContent
) {
    auto localeCode = player.getLocaleCode();

    std::string content = baseContent
                        + "\n\n租赁天数: {}\n总租金: {}\n{}"_trl(
                              localeCode,
                              days,
                              totalPrice,
                              EconomySystem::getInstance().getCostMessage(player, totalPrice)
                        );

    auto confirm = ll::form::SimpleForm{};
    confirm.setTitle("[PLand] | 确认租赁"_trl(localeCode));
    confirm.setContent(content);
    confirm.appendButton(
        "确认租赁"_trl(localeCode),
        "textures/ui/realms_green_check",
        "path",
        [selector, days](Player& pl2) {
            auto& service = PLand::getInstance().getServiceLocator().getLeasingService();
            if (auto exp = service.leaseLand(pl2, selector, days)) {
                feedback_utils::notifySuccess(pl2, "租赁领地成功"_trl(pl2.getLocaleCode()));
            } else {
                feedback_utils::sendError(pl2, exp.error());
            }
        }
    );
    back_utils::injectBackButton(confirm, back_utils::wrapCallback<_chooseLeaseDays>(selector));
    confirm.sendTo(player);
}

void LandBuyGUI::_impl(Player& player, LandResizeSelector* selector) {
    auto localeCode = player.getLocaleCode();

    auto aabb = selector->newLandAABB();

    aabb->fix();
    auto const volume = aabb->getVolume();
    if (volume >= INT_MAX) {
        feedback_utils::sendErrorText(player, "领地体积过大，无法购买"_trl(localeCode));
        return;
    }

    auto      land          = selector->getLand();
    int const originalPrice = land->getOriginalBuyPrice(); // 原始购买价格

    std::string content = "体积: {0}x{1}x{2} = {3}\n范围: {4}\n原购买价格: {5}"_trl(
        localeCode,
        aabb->getBlockCountX(),
        aabb->getBlockCountZ(),
        aabb->getBlockCountY(),
        volume,
        aabb->toString(),
        originalPrice
    );

    std::optional<int64_t> discountedPrice;
    std::optional<int64_t> needPay;
    std::optional<int64_t> refund;
    if (ConfigProvider::isEconomySystemEnabled()) {
        auto& service = PLand::getInstance().getServiceLocator().getLandPriceService();
        if (auto result = service.getOrdinaryLandPrice(*aabb, land->getDimensionId(), land->is3D())) {
            discountedPrice  = result->mDiscountedPrice;
            needPay          = result->mDiscountedPrice - originalPrice;
            refund           = originalPrice - result->mDiscountedPrice;
            content         += "\n需补差价: {0}\n需退差价: {1}\n{2}"_trl(
                localeCode,
                needPay.value_or(0) < 0 ? 0 : needPay,
                refund.value_or(0) < 0 ? 0 : refund,
                needPay.value_or(0) > 0 ? EconomySystem::getInstance().getCostMessage(player, needPay.value()) : ""
            );
        } else {
            feedback_utils::sendErrorText(player, "价格表达式解析失败，请联系管理员"_trl(localeCode));
            PLand::getInstance().getSelf().getLogger().error(result.error().message());
            return;
        }
    }

    auto fm = ll::form::SimpleForm{};
    fm.setTitle("[PLand] | 购买领地 & 重新选区"_trl(localeCode));
    fm.setContent(content);
    fm.appendButton(
        "确认购买"_trl(localeCode),
        "textures/ui/realms_green_check",
        "path",
        [needPay, refund, discountedPrice, selector](Player& pl) {
            LandResizeSettlement settlement{};
            settlement.newTotalPrice = discountedPrice.value_or(0);
            if (needPay.value_or(0) > 0) {
                settlement.type   = LandResizeSettlement::Type::Pay;
                settlement.amount = needPay.value();
            } else if (refund.value_or(0) > 0) {
                settlement.type   = LandResizeSettlement::Type::Refund;
                settlement.amount = refund.value();
            } else {
                settlement.type = LandResizeSettlement::Type::NoChange;
            }

            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
            if (auto exp = service.handleChangeRange(pl, selector, settlement)) {
                feedback_utils::sendText(pl, "领地范围修改成功"_trl(pl.getLocaleCode()));
            } else {
                feedback_utils::sendError(pl, exp.error());
            }
        }
    );
    fm.appendButton("暂存订单"_trl(localeCode), "textures/ui/recipe_book_icon", "path"); // close
    fm.appendButton("放弃订单"_trl(localeCode), "textures/ui/cancel", "path", [](Player& pl) {
        land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
    });

    fm.sendTo(player);
}

void LandBuyGUI::_impl(Player& player, SubLandCreateSelector* selector) {
    auto localeCode = player.getLocaleCode();

    auto subLandRange = selector->newLandAABB();
    subLandRange->fix();
    auto const volume = subLandRange->getVolume();
    if (volume >= INT_MAX) {
        feedback_utils::sendErrorText(player, "领地体积过大，无法购买"_trl(localeCode));
        return;
    }

    auto&       parentPos = selector->getParentLand()->getAABB();
    std::string content   = "[父领地]\n体积: {}x{}x{}={}\n范围: {}\n\n[子领地]\n体积: {}x{}x{}={}\n范围: {}"_trl(
        localeCode,
        // 父领地
        parentPos.getBlockCountX(),
        parentPos.getBlockCountZ(),
        parentPos.getBlockCountY(),
        parentPos.getVolume(),
        parentPos.toString(),
        // 子领地
        subLandRange->getBlockCountX(),
        subLandRange->getBlockCountZ(),
        subLandRange->getBlockCountY(),
        volume,
        subLandRange->toString()
    );

    std::optional<int64_t> discountedPrice;
    if (ConfigProvider::isEconomySystemEnabled()) {
        auto& service = PLand::getInstance().getServiceLocator().getLandPriceService();
        if (auto result = service.getSubLandPrice(*subLandRange, selector->getParentLand()->getDimensionId())) {
            discountedPrice  = result->mDiscountedPrice;
            content         += "\n\n[价格]\n原价: {}\n折扣价: {}\n{}"_trl(
                localeCode,
                // 价格
                result->mOriginalPrice,
                result->mDiscountedPrice,
                EconomySystem::getInstance().getCostMessage(player, result->mDiscountedPrice)
            );
        } else {
            feedback_utils::sendErrorText(player, "价格表达式解析失败，请联系管理员"_trl(localeCode));
            PLand::getInstance().getSelf().getLogger().error(result.error().message());
            return;
        }
    }

    auto fm = ll::form::SimpleForm{};
    fm.setTitle("[PLand] | 购买领地 & 子领地"_trl(localeCode));
    fm.setContent(content);
    fm.appendButton(
        "确认购买"_trl(localeCode),
        "textures/ui/realms_green_check",
        "path",
        [discountedPrice, selector](Player& pl) {
            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();
            if (auto exp = service.buyLand(pl, selector, discountedPrice.value_or(0))) {
                feedback_utils::notifySuccess(pl, "购买领地成功"_trl(pl.getLocaleCode()));
            } else {
                feedback_utils::sendError(pl, exp.error());
            }
        }
    );
    fm.appendButton("暂存订单"_trl(localeCode), "textures/ui/recipe_book_icon", "path"); // close
    fm.appendButton("放弃订单"_trl(localeCode), "textures/ui/cancel", "path", [](Player& pl) {
        land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
    });

    fm.sendTo(player);
}

} // namespace land::gui
