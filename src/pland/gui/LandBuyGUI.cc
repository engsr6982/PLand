#include "pland/gui/LandBuyGUI.h"
#include "ll/api/event/EventBus.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/economy/EconomySystem.h"
#include "pland/economy/PriceCalculate.h"
#include "pland/gui/form/BackSimpleForm.h"
#include "pland/infra/Config.h"
#include "pland/land/Land.h"
#include "pland/land/LandCreateValidator.h"
#include "pland/land/LandEvent.h"
#include "pland/land/LandRegistry.h"
#include "pland/land/StorageLayerError.h"
#include "pland/selector/ChangeLandRangeSelector.h"
#include "pland/selector/DefaultSelector.h"
#include "pland/selector/SelectorManager.h"
#include "pland/selector/SubLandSelector.h"
#include "pland/utils/McUtils.h"
#include <climits>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>


namespace land {

void LandBuyGUI::impl(Player& player) {
    auto manager = land::PLand::getInstance().getSelectorManager();
    if (!manager->hasSelector(player)) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_trf(player));
        return;
    }

    auto selector = manager->getSelector(player);
    if (auto def = selector->as<DefaultSelector>()) {
        impl(player, def);
    } else if (auto re = selector->as<ChangeLandRangeSelector>()) {
        impl(player, re);
    } else if (auto sub = selector->as<SubLandSelector>()) {
        impl(player, sub);
    }
}

void LandBuyGUI::impl(Player& player, DefaultSelector* selector) {
    bool const& is3D = selector->is3D();
    auto        aabb = selector->newLandAABB();

    aabb->fix();
    int const volume = aabb->getVolume();
    int const length = aabb->getDepth();
    int const width  = aabb->getWidth();
    int const height = aabb->getHeight();

    auto   _variables    = PriceCalculate::Variable::make(*aabb, selector->getDimensionId()); // 传入维度ID
    double originalPrice = PriceCalculate::eval(
        is3D ? Config::cfg.land.bought.threeDimensionl.calculate : Config::cfg.land.bought.twoDimensionl.calculate,
        _variables
    );

    // 应用维度价格系数
    auto it = Config::cfg.land.bought.dimensionPriceCoefficients.find(std::to_string(selector->getDimensionId()));
    if (it != Config::cfg.land.bought.dimensionPriceCoefficients.end()) {
        originalPrice *= it->second;
    }
    int discountedPrice = PriceCalculate::calculateDiscountPrice(originalPrice, Config::cfg.land.discountRate);
    if (!Config::cfg.economy.enabled) discountedPrice = 0; // 免费

    if (volume >= INT_MAX || originalPrice < 0 || discountedPrice < 0) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地体积过大，无法购买"_trf(player));
        return;
    }

    // publish event
    PlayerBuyLandBeforeEvent ev(player, selector, discountedPrice);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto fm = BackSimpleForm<>::make();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地"_trf(player)));
    fm.setContent("领地类型: {}\n体积: {}x{}x{} = {}\n范围: {}\n原价: {}\n折扣价: {}\n{}"_trf(
        player,
        is3D ? "3D" : "2D",
        length,
        width,
        height,
        volume,
        aabb->toString(),
        originalPrice,
        discountedPrice,
        EconomySystem::getInstance()->getCostMessage(player, discountedPrice)
    ));

    fm.appendButton(
        "确认购买"_trf(player),
        "textures/ui/realms_green_check",
        "path",
        [discountedPrice, selector](Player& pl) {
            auto& economy = EconomySystem::getInstance();
            if (economy->get(pl) < discountedPrice && Config::cfg.economy.enabled) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return; // 预检查经济
            }

            SharedLand landPtr = selector->newLand();

            if (auto res = LandCreateValidator::validateCreateOrdinaryLand(pl, landPtr); !res) {
                LandCreateValidator::sendErrorMessage(pl, res.error());
                return;
            }

            // 扣除经济
            if (!economy->reduce(pl, discountedPrice)) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return;
            }

            landPtr->setOriginalBuyPrice(discountedPrice);

            if (auto res = PLand::getInstance().getLandRegistry()->addOrdinaryLand(landPtr); !res) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "购买领地失败"_trf(pl));
                StorageLayerError::sendErrorMessage(pl, res.error());
                (void)economy->add(pl, discountedPrice); // 补回经济
                return;
            }

            land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
            ll::event::EventBus::getInstance().publish(PlayerBuyLandAfterEvent{pl, landPtr});

            mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "购买领地成功"_trf(pl));
        }
    );
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon", "path"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", "path", [](Player& pl) {
        land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
    });

    fm.sendTo(player);
}

void LandBuyGUI::impl(Player& player, ChangeLandRangeSelector* reSelector) {
    auto aabb = reSelector->newLandAABB();

    aabb->fix();
    int const volume = aabb->getVolume();
    int const length = aabb->getDepth();
    int const width  = aabb->getWidth();
    int const height = aabb->getHeight();

    auto       landPtr       = reSelector->getLand();
    int const& originalPrice = landPtr->getOriginalBuyPrice(); // 原始购买价格
    auto       _variables    = PriceCalculate::Variable::make(*aabb, landPtr->getDimensionId());
    double     newRangePrice = PriceCalculate::eval(Config::cfg.land.bought.threeDimensionl.calculate, _variables);

    // 应用维度价格系数
    auto it = Config::cfg.land.bought.dimensionPriceCoefficients.find(std::to_string(landPtr->getDimensionId()));
    if (it != Config::cfg.land.bought.dimensionPriceCoefficients.end()) {
        newRangePrice *= it->second;
    }

    int const discountedPrice =
        PriceCalculate::calculateDiscountPrice(newRangePrice, Config::cfg.land.discountRate); // 新范围购买价格

    // 计算需补差价 & 退还差价
    int needPay = discountedPrice - originalPrice; // 需补差价
    int refund  = originalPrice - discountedPrice; // 退还差价

    if (volume >= INT_MAX || originalPrice < 0 || discountedPrice < 0) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地体积过大，无法购买"_trf(player));
        return;
    }

    // publish event
    LandRangeChangeBeforeEvent ev(player, landPtr, *aabb, needPay, refund);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto fm = BackSimpleForm<>::make();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地 & 重新选区"_trf(player)));
    fm.setContent("体积: {0}x{1}x{2} = {3}\n范围: {4}\n原购买价格: {5}\n需补差价: {6}\n需退差价: {7}\n{8}"_trf(
        player,
        length,                    // 1
        width,                     // 2
        height,                    // 3
        volume,                    // 4
        aabb->toString(),          // 5
        originalPrice,             // 6
        needPay < 0 ? 0 : needPay, // 7
        refund < 0 ? 0 : refund,   // 8
        needPay > 0 ? EconomySystem::getInstance()->getCostMessage(player, needPay) : ""
    ));

    fm.appendButton(
        "确认购买"_trf(player),
        "textures/ui/realms_green_check",
        "path",
        [needPay, refund, discountedPrice, aabb, reSelector, landPtr](Player& pl) {
            auto& eco = EconomySystem::getInstance();
            if ((needPay > 0 && eco->get(pl) < needPay) && Config::cfg.economy.enabled) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return; // 预检查经济
            }

            if (auto res = LandCreateValidator::validateChangeLandRange(landPtr, *aabb); !res) {
                LandCreateValidator::sendErrorMessage(pl, res.error());
                return;
            }

            // 补差价 & 退还差价
            if (needPay > 0) {
                if (!eco->reduce(pl, needPay)) {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                    return;
                }
            } else if (refund > 0) {
                if (!eco->add(pl, refund)) {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "经济系统异常,退还差价失败"_trf(pl));
                    return;
                }
            }

            if (!landPtr->setAABB(*aabb)) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地范围修改失败"_trf(pl));
                return;
            }

            landPtr->setOriginalBuyPrice(discountedPrice);
            PLand::getInstance().getLandRegistry()->refreshLandRange(landPtr); // 刷新领地范围

            land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
            ll::event::EventBus::getInstance().publish(LandRangeChangeAfterEvent{pl, landPtr, *aabb, needPay, refund});
            mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "领地范围修改成功"_trf(pl));
        }
    );
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon", "path"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", "path", [](Player& pl) {
        land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
    });

    fm.sendTo(player);
}

void LandBuyGUI::impl(Player& player, SubLandSelector* subSelector) {
    auto aabb = subSelector->newLandAABB();

    aabb->fix();
    int const volume = aabb->getVolume();
    int const length = aabb->getDepth();
    int const width  = aabb->getWidth();
    int const height = aabb->getHeight();

    auto   _variables    = PriceCalculate::Variable::make(*aabb, subSelector->getDimensionId()); // 传入维度ID
    double originalPrice = PriceCalculate::eval(Config::cfg.land.subLand.calculate, _variables);

    // 应用维度价格系数
    auto it = Config::cfg.land.bought.dimensionPriceCoefficients.find(std::to_string(subSelector->getDimensionId()));
    if (it != Config::cfg.land.bought.dimensionPriceCoefficients.end()) {
        originalPrice *= it->second;
    }
    int discountedPrice = PriceCalculate::calculateDiscountPrice(originalPrice, Config::cfg.land.discountRate);
    if (!Config::cfg.economy.enabled) discountedPrice = 0; // 免费

    if (volume >= INT_MAX || originalPrice < 0 || discountedPrice < 0) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地体积过大，无法购买"_trf(player));
        return;
    }

    // publish event
    PlayerBuyLandBeforeEvent ev(player, subSelector, discountedPrice);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto fm = BackSimpleForm<>::make();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地 & 子领地"_trf(player)));

    auto& parentPos = subSelector->getParentLand()->getAABB();
    fm.setContent(
        "[父领地]\n体积: {}x{}x{}={}\n范围: {}\n\n[子领地]\n体积: {}x{}x{}={}\n范围: {}\n\n[价格]\n原价: {}\n折扣价: {}\n{}"_trf(
            player,
            // 父领地
            parentPos.getDepth(),
            parentPos.getWidth(),
            parentPos.getHeight(),
            parentPos.getVolume(),
            parentPos.toString(),
            // 子领地
            length,
            width,
            height,
            volume,
            aabb->toString(),
            // 价格
            originalPrice,
            discountedPrice,
            EconomySystem::getInstance()->getCostMessage(player, discountedPrice)
        )
    );

    fm.appendButton(
        "确认购买"_trf(player),
        "textures/ui/realms_green_check",
        "path",
        [discountedPrice, aabb, subSelector, &parentPos](Player& pl) {
            auto& economy = EconomySystem::getInstance();
            if (economy->get(pl) < discountedPrice && Config::cfg.economy.enabled) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return; // 预检查经济
            }

            if (auto res = LandCreateValidator::validateCreateSubLand(pl, subSelector->getParentLand(), *aabb); !res) {
                LandCreateValidator::sendErrorMessage(pl, res.error());
                return;
            }

            // 扣除经济
            if (!economy->reduce(pl, discountedPrice)) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return;
            }

            // 创建领地
            SharedLand subLand = subSelector->newSubLand();
            subLand->setOriginalBuyPrice(discountedPrice); // 保存购买价格

            if (!PLand::getInstance().getLandRegistry()->addSubLand(subSelector->getParentLand(), subLand)) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地创建失败，请重试"_trf(pl));
                (void)economy->add(pl, discountedPrice); // 补回经济
                return;
            }

            land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
            ll::event::EventBus::getInstance().publish(PlayerBuyLandAfterEvent{pl, subLand});
            mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "购买领地成功"_trf(pl));
        }
    );
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon", "path"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", "path", [](Player& pl) {
        land::PLand::getInstance().getSelectorManager()->stopSelection(pl);
    });

    fm.sendTo(player);
}

} // namespace land
