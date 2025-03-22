#include "pland/gui/BuyLandGui.h"
#include "ll/api/event/EventBus.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Config.h"
#include "pland/EconomySystem.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandEvent.h"
#include "pland/LandPos.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/PriceCalculate.h"
#include "pland/utils/McUtils.h"
#include "pland/wrapper/FormEx.h"
#include <climits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>


namespace land {

void BuyLandGui::impl(Player& player) {
    auto selector = SelectorManager::getInstance().get(player);
    if (!selector) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_trf(player));
        return;
    }

    switch (selector->getType()) {
    case Selector::Type::Default: {
        impl(player, selector);
        break;
    }

    case Selector::Type::ReSelector: {
        if (auto reSelector = selector->As<LandReSelector>()) {
            impl(player, reSelector);
        }
        break;
    }

    case Selector::Type::SubLand: {
        if (auto subSelector = selector->As<SubLandSelector>()) {
            impl(player, subSelector);
        }
        break;
    }
    }
}

void BuyLandGui::impl(Player& player, Selector* selector) {
    if (selector->getType() != Selector::Type::Default) {
        throw std::runtime_error("invalid selector type");
        return;
    }

    bool const& is3D = selector->is3D();
    auto        aabb = selector->getAABB();

    aabb->fix();
    int const volume = aabb->getVolume();
    int const length = aabb->getDepth();
    int const width  = aabb->getWidth();
    int const height = aabb->getHeight();

    auto   _variables    = PriceCalculate::Variable::make(*aabb);
    double originalPrice = PriceCalculate::eval(
        is3D ? Config::cfg.land.bought.threeDimensionl.calculate : Config::cfg.land.bought.twoDimensionl.calculate,
        _variables
    );
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

    auto fm = SimpleFormEx::create();
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
        EconomySystem::getInstance().getSpendTip(player, discountedPrice)
    ));

    fm.appendButton(
        "确认购买"_trf(player),
        "textures/ui/realms_green_check",
        [discountedPrice, aabb, length, width, height, is3D, selector](Player& pl) {
            auto& economy = EconomySystem::getInstance();
            if (economy.get(pl) < discountedPrice && Config::cfg.economy.enabled) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return; // 预检查经济
            }

            auto& db = PLand::getInstance();
            if ((int)db.getLands(pl.getUuid().asString()).size() >= Config::cfg.land.maxLand) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您已经达到最大领地数量"_trf(pl));
                return;
            }

            auto const& squareRange = Config::cfg.land.bought.squareRange;
            if ((length < squareRange.min || width < squareRange.min) || // 长度和宽度必须大于最小值
                (length > squareRange.max || width > squareRange.max) || // 长度和宽度必须小于最大值
                (is3D && (height < squareRange.minHeight || height > mc_utils::GetDimensionMaxHeight(pl.getDimension()))
                ) // 高度必须大于最小值 && 小于世界高度 && 3D
            ) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(
                    pl,
                    "领地范围不合法, 可用范围: 长宽: {}~{} 最小高度: {}, 当前长宽高: {}x{}x{}"_trf(
                        pl,
                        squareRange.min,
                        squareRange.max,
                        squareRange.minHeight,
                        length,
                        width,
                        height
                    )
                );
                return;
            }

            auto lands = db.getLandAt(aabb->mMin_A, aabb->mMax_B, selector->getDimensionId());
            if (!lands.empty()) {
                for (auto& land : lands) {
                    if (LandPos::isCollision(land->mPos, *aabb)) {
                        mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地重叠，请重新选择"_trf(pl));
                        return;
                    }
                    if (!LandPos::isComplisWithMinSpacing(land->mPos, *aabb, Config::cfg.land.minSpacing)) {
                        mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地距离过近，请重新选择"_trf(pl));
                        return;
                    }
                }
            }

            // 扣除经济
            if (!economy.reduce(pl, discountedPrice)) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return;
            }

            auto landPtr = selector->newLandData();
            if (db.addLand(landPtr)) {
                landPtr->mOriginalBuyPrice = discountedPrice; // 保存购买价格
                mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "购买领地成功"_trf(pl));

                PlayerBuyLandAfterEvent ev(pl, landPtr);
                ll::event::EventBus::getInstance().publish(ev);

                SelectorManager::getInstance().cancel(pl);

            } else {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "购买领地失败"_trf(pl));
                economy.add(pl, discountedPrice); // 补回经济
            }
        }
    );
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", [](Player& pl) {
        SelectorManager::getInstance().cancel(pl);
    });

    fm.sendTo(player);
}

void BuyLandGui::impl(Player& player, LandReSelector* reSelector) {
    bool const& is3D = reSelector->is3D();
    auto        aabb = reSelector->getAABB();

    aabb->fix();
    int const volume = aabb->getVolume();
    int const length = aabb->getDepth();
    int const width  = aabb->getWidth();
    int const height = aabb->getHeight();

    auto       landPtr         = reSelector->getLandData();
    int const& originalPrice   = landPtr->mOriginalBuyPrice;             // 原始购买价格
    int const  discountedPrice = PriceCalculate::calculateDiscountPrice( // 新范围购买价格
        PriceCalculate::eval(
            is3D ? Config::cfg.land.bought.threeDimensionl.calculate : Config::cfg.land.bought.twoDimensionl.calculate,
            PriceCalculate::Variable::make(*aabb)
        ),
        Config::cfg.land.discountRate
    );

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

    auto fm = SimpleFormEx::create();
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
        needPay > 0 ? EconomySystem::getInstance().getSpendTip(player, discountedPrice) : ""
    ));

    fm.appendButton(
        "确认购买"_trf(player),
        "textures/ui/realms_green_check",
        [needPay, refund, discountedPrice, aabb, length, width, height, is3D, reSelector, landPtr](Player& pl) {
            auto& eco = EconomySystem::getInstance();
            if ((needPay > 0 && eco.get(pl) < needPay) && Config::cfg.economy.enabled) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return; // 预检查经济
            }


            auto const& squareRange = Config::cfg.land.bought.squareRange;
            if ((length < squareRange.min || width < squareRange.min) || // 长度和宽度必须大于最小值
                (length > squareRange.max || width > squareRange.max) || // 长度和宽度必须小于最大值
                (is3D && (height < squareRange.minHeight || height > mc_utils::GetDimensionMaxHeight(pl.getDimension()))
                ) // 高度必须大于最小值 && 小于世界高度 && 3D
            ) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(
                    pl,
                    "领地范围不合法, 可用范围: 长宽: {}~{} 最小高度: {}"_trf(
                        pl,
                        squareRange.min,
                        squareRange.max,
                        squareRange.minHeight
                    )
                );
                return;
            }

            auto& db    = PLand::getInstance();
            auto  lands = db.getLandAt(aabb->mMin_A, aabb->mMax_B, landPtr->getLandDimid());
            if (!lands.empty()) {
                for (auto& land : lands) {
                    if (land->getLandID() == landPtr->getLandID()) continue; // 跳过自己
                    if (LandPos::isCollision(land->mPos, *aabb)) {
                        mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地重叠，请重新选择"_trf(pl));
                        return;
                    }
                    if (!LandPos::isComplisWithMinSpacing(land->mPos, *aabb, Config::cfg.land.minSpacing)) {
                        mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地距离过近，请重新选择"_trf(pl));
                        return;
                    }
                }
            }

            // 补差价 & 退还差价
            if (needPay > 0) {
                if (!eco.reduce(pl, needPay)) {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                    return;
                }
            } else if (refund > 0) {
                if (!eco.add(pl, refund)) {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "经济系统异常,退还差价失败"_trf(pl));
                    return;
                }
            }

            if (landPtr->_setLandPos(*aabb)) {
                landPtr->mOriginalBuyPrice = discountedPrice; // 保存购买价格

                db.refreshLandRange(landPtr); // 刷新领地范围

                mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "领地范围修改成功"_trf(pl));

                LandRangeChangeAfterEvent ev(pl, landPtr, *aabb, needPay, refund);
                ll::event::EventBus::getInstance().publish(ev);

                SelectorManager::getInstance().cancel(pl);

            } else mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地范围修改失败"_trf(pl));
        }
    );
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", [](Player& pl) {
        SelectorManager::getInstance().cancel(pl);
    });

    fm.sendTo(player);
}

void BuyLandGui::impl(Player& player, SubLandSelector* subSelector) {
    bool const& is3D = subSelector->is3D();
    auto        aabb = subSelector->getAABB();

    aabb->fix();
    int const volume = aabb->getVolume();
    int const length = aabb->getDepth();
    int const width  = aabb->getWidth();
    int const height = aabb->getHeight();

    auto   _variables      = PriceCalculate::Variable::make(*aabb);
    double originalPrice   = PriceCalculate::eval(Config::cfg.land.subLand.calculate, _variables);
    int    discountedPrice = PriceCalculate::calculateDiscountPrice(originalPrice, Config::cfg.land.discountRate);
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

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地 & 子领地"_trf(player)));

    auto& parentPos = subSelector->getParentLandData()->getLandPos();
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
            EconomySystem::getInstance().getSpendTip(player, discountedPrice)
        )
    );

    fm.appendButton(
        "确认购买"_trf(player),
        "textures/ui/realms_green_check",
        [discountedPrice, aabb, length, width, height, is3D, subSelector, &parentPos](Player& pl) {
            auto& economy = EconomySystem::getInstance();
            if (economy.get(pl) < discountedPrice && Config::cfg.economy.enabled) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return; // 预检查经济
            }

            auto& db = PLand::getInstance();
            if ((int)db.getLands(pl.getUuid().asString()).size() >= Config::cfg.land.maxLand) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您已经达到最大领地数量"_trf(pl));
                return;
            }

            auto const& squareRange = Config::cfg.land.bought.squareRange;
            if ((length < squareRange.min || width < squareRange.min) || // 长度和宽度必须大于最小值
                (length > squareRange.max || width > squareRange.max) || // 长度和宽度必须小于最大值
                (is3D && (height < squareRange.minHeight || height > mc_utils::GetDimensionMaxHeight(pl.getDimension()))
                ) // 高度必须大于最小值 && 小于世界高度 && 3D
            ) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(
                    pl,
                    "领地范围不合法, 可用范围: 长宽: {}~{} 最小高度: {}, 当前长宽高: {}x{}x{}"_trf(
                        pl,
                        squareRange.min,
                        squareRange.max,
                        squareRange.minHeight,
                        length,
                        width,
                        height
                    )
                );
                return;
            }

            if (!LandPos::isContain(parentPos, *aabb)) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "子领地不在父领地范围内"_trf(pl));
                return;
            }

            auto parentLand = subSelector->getParentLandData();
            if (parentLand->isOrdinaryLand()) {
                // 普通领地，查询其它领地进行碰撞检测
                auto lands = db.getLandAt(aabb->mMin_A, aabb->mMax_B, subSelector->getDimensionId());
                if (!lands.empty()) {
                    for (auto& land : lands) {
                        if (land == parentLand) continue; // 当前领地在在父领地内部，跳过检测
                        if (LandPos::isCollision(land->mPos, *aabb)) {
                            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地重叠，请重新选择"_trf(pl));
                            return;
                        }
                        if (!LandPos::isComplisWithMinSpacing(land->mPos, *aabb, Config::cfg.land.subLand.minSpacing)) {
                            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地距离过近，请重新选择"_trf(pl));
                            return;
                        }
                    }
                }

            } else if (parentLand->isMixLand() || parentLand->isSubLand()) {
                auto lands = parentLand->getSubLands();
                if (!lands.empty()) {
                    for (auto& land : lands) {
                        if (LandPos::isCollision(land->mPos, *aabb)) {
                            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地重叠，请重新选择"_trf(pl));
                            return;
                        }
                        if (!LandPos::isComplisWithMinSpacing(land->mPos, *aabb, Config::cfg.land.subLand.minSpacing)) {
                            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地距离过近，请重新选择"_trf(pl));
                            return;
                        }
                    }
                }
            }

            // 扣除经济
            if (!economy.reduce(pl, discountedPrice)) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return;
            }

            // 创建领地
            auto landPtr = subSelector->newLandData();
            if (db.addLand(landPtr)) {
                SelectorManager::getInstance().cancel(pl);

                landPtr->mOriginalBuyPrice = discountedPrice;            // 保存购买价格
                parentLand->mSubLandIDs.push_back(landPtr->getLandID()); // 添加子领地
                landPtr->mParentLandID = parentLand->getLandID();        // 设置父领地

                PlayerBuyLandAfterEvent ev(pl, landPtr);
                ll::event::EventBus::getInstance().publish(ev);

                mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "购买领地成功"_trf(pl));

            } else {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "购买领地失败"_trf(pl));
                economy.add(pl, discountedPrice); // 补回经济
            }
        }
    );
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", [](Player& pl) {
        SelectorManager::getInstance().cancel(pl);
    });

    fm.sendTo(player);
}

} // namespace land