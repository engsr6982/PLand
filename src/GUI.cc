#include "pland/GUI.h"
#include "fmt/compile.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/PlayerInfo.h"
#include "mc/world/actor/player/Player.h"
#include "mod/MyMod.h"
#include "pland/Calculate.h"
#include "pland/Config.h"
#include "pland/EconomySystem.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandEvent.h"
#include "pland/LandPos.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/SafeTeleport.h"
#include "pland/utils/JSON.h"
#include "pland/utils/McUtils.h"
#include "pland/utils/Utils.h"
#include "pland/wrapper/FormEx.h"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>


using namespace ll::form;


namespace land {

// 独立GUI
void ChooseLandDimAndNewLand::impl(Player& player) {
    if (!some(Config::cfg.land.bought.allowDimensions, player.getDimensionId().id)) {
        mc_utils::sendText(player, "你所在的维度无法购买领地"_trf(player));
        return;
    }

    PlayerAskCreateLandBeforeEvent ev(player);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    ModalForm(
        PLUGIN_NAME + ("| 选择领地维度"_trf(player)),
        "请选择领地维度\n\n2D: 领地拥有整个Y轴\n3D: 自行选择Y轴范围"_trf(player),
        "2D", // true
        "3D"  // false
    )
        .sendTo(player, [](Player& pl, ModalFormResult res, FormCancelReason) {
            if (!res.has_value()) {
                return;
            }

            bool land3D = !((bool)res.value());
            if (land3D && !Config::cfg.land.bought.threeDimensionl.enabled) {
                mc_utils::sendText(pl, "3D领地功能未启用，请联系管理员"_trf(pl));
                return;
            }
            if (!land3D && !Config::cfg.land.bought.twoDimensionl.enabled) {
                mc_utils::sendText(pl, "2D领地功能未启用，请联系管理员"_trf(pl));
                return;
            }

            PlayerAskCreateLandAfterEvent ev(pl, land3D);
            ll::event::EventBus::getInstance().publish(ev);

            LandSelector::getInstance().tryStartSelect(pl, pl.getDimensionId(), land3D);
            mc_utils::sendText(
                pl,
                "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trf(pl, Config::cfg.selector.tool)
            );
        });
}
void LandBuyGui::impl(Player& player) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_trf(player));
        return;
    }

    if (dataPtr->mBindLandData.lock()) {
        LandBuyWithReSelectGui::impl(player); // 重新选择领地，切换到另一个购买界面
        return;
    }

    bool const& is3D = dataPtr->mDraw3D;

    dataPtr->mPos.fix();
    int const volume = dataPtr->mPos.getVolume();
    if (volume >= INT_MAX) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地体积过大，无法购买"_trf(player));
        return;
    }

    auto& economy = EconomySystem::getInstance();

    int originalPrice = Calculate(dataPtr->mPos)
                            .eval(
                                is3D ? Config::cfg.land.bought.threeDimensionl.calculate
                                     : Config::cfg.land.bought.twoDimensionl.calculate
                            );
    int discountedPrice = Calculate::calculateDiscountPrice(originalPrice, Config::cfg.land.discountRate);

    if (originalPrice < 0 || discountedPrice < 0) {
        // 范围过大导致溢出
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地体积过大，无法购买"_trf(player));
        return;
    }

    if (!Config::cfg.economy.enabled) {
        discountedPrice = 0; // 免费
    }

    // publish event
    PlayerBuyLandBeforeEvent ev(player, dataPtr, discountedPrice);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地"_trf(player)));
    fm.setContent("领地类型: {}\n体积: {}x{}x{} = {}\n范围: {}\n原价: {}\n折扣价: {}\n{}"_trf(
        player,
        is3D ? "3D" : "2D",
        dataPtr->mPos.getDepth(),
        dataPtr->mPos.getWidth(),
        dataPtr->mPos.getHeight(),
        volume,
        dataPtr->mPos.toString(),
        originalPrice,
        discountedPrice,
        economy.getSpendTip(player, discountedPrice)
    ));

    fm.appendButton("确认购买"_trf(player), "textures/ui/realms_green_check", [discountedPrice](Player& pl) {
        auto& eco = EconomySystem::getInstance();
        if (eco.get(pl) < discountedPrice && Config::cfg.economy.enabled) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
            return; // 预检查经济
        }

        auto& db = PLand::getInstance();
        if ((int)db.getLands(pl.getUuid().asString()).size() >= Config::cfg.land.maxLand) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您已经达到最大领地数量"_trf(pl));
            return;
        }

        auto& selector = LandSelector::getInstance();
        auto  data     = selector.getSelector(pl);
        if (!data) {
            return;
        }

        int const   length      = data->mPos.getDepth();
        int const   width       = data->mPos.getWidth();
        int const   height      = data->mPos.getHeight();
        auto const& squareRange = Config::cfg.land.bought.squareRange;
        if ((length < squareRange.min || width < squareRange.min) || // 长度和宽度必须大于最小值
            (length > squareRange.max || width > squareRange.max) || // 长度和宽度必须小于最大值
            (data->mDraw3D
             && (height < squareRange.minHeight || height > mc_utils::GetDimensionMaxHeight(pl.getDimension()))
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

        auto lands = db.getLandAt(data->mPos.mMin_A, data->mPos.mMax_B, data->mDimid);
        if (!lands.empty()) {
            for (auto& land : lands) {
                if (LandPos::isCollision(land->mPos, data->mPos)) {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地重叠，请重新选择"_trf(pl));
                    return;
                }
                if (!LandPos::isComplisWithMinSpacing(land->mPos, data->mPos)) {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地距离过近，请重新选择"_trf(pl));
                    return;
                }
            }
        }

        // 扣除经济
        if (!eco.reduce(pl, discountedPrice)) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
            return;
        }

        auto landPtr = selector.makeLandFromSelector(pl);
        if (db.addLand(landPtr)) {
            landPtr->mOriginalBuyPrice = discountedPrice; // 保存购买价格
            selector.completeAndRelease(pl);
            mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "购买领地成功"_trf(pl));

            PlayerBuyLandAfterEvent ev(pl, landPtr);
            ll::event::EventBus::getInstance().publish(ev);

        } else {
            mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "购买领地失败"_trf(pl));
            eco.add(pl, discountedPrice); // 补回经济
        }
    });
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", [](Player& pl) {
        LandSelector::getInstance().tryCancel(pl);
    });

    fm.sendTo(player);
}
void LandBuyGui::LandBuyWithReSelectGui::impl(Player& player) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_trf(player));
        return;
    }

    auto&       economy = EconomySystem::getInstance();
    bool const& is3D    = dataPtr->mDraw3D;

    dataPtr->mPos.fix();
    int const volume = dataPtr->mPos.getVolume();
    if (volume >= INT_MAX) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地体积过大，无法购买"_trf(player));
        return;
    }

    int const& originalPrice   = dataPtr->mBindLandData.lock()->mOriginalBuyPrice; // 原始购买价格
    int const  discountedPrice = Calculate::calculateDiscountPrice(                // 新范围购买价格
        Calculate(dataPtr->mPos)
            .eval(
                is3D ? Config::cfg.land.bought.threeDimensionl.calculate
                     : Config::cfg.land.bought.twoDimensionl.calculate
            ),
        Config::cfg.land.discountRate
    );

    // 计算需补差价 & 退还差价
    int needPay = discountedPrice - originalPrice; // 需补差价
    int refund  = originalPrice - discountedPrice; // 退还差价


    // publish event
    LandRangeChangeBeforeEvent ev(player, dataPtr->mBindLandData.lock(), dataPtr->mPos, needPay, refund);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地 & 重新选区"_trf(player)));
    fm.setContent("体积: {0}x{1}x{2} = {3}\n范围: {4}\n原购买价格: {5}\n需补差价: {6}\n需退差价: {7}\n{8}"_trf(
        player,
        dataPtr->mPos.getDepth(),  // 1
        dataPtr->mPos.getWidth(),  // 2
        dataPtr->mPos.getHeight(), // 3
        dataPtr->mPos.getVolume(), // 4
        dataPtr->mPos.toString(),  // 5
        originalPrice,             // 6
        needPay < 0 ? 0 : needPay, // 7
        refund < 0 ? 0 : refund,   // 8
        needPay > 0 ? economy.getSpendTip(player, discountedPrice) : ""
    ));

    fm.appendButton(
        "确认购买"_trf(player),
        "textures/ui/realms_green_check",
        [needPay, refund, discountedPrice](Player& pl) {
            auto& eco = EconomySystem::getInstance();
            if ((needPay > 0 && eco.get(pl) < needPay) && Config::cfg.economy.enabled) {
                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "您的余额不足，无法购买"_trf(pl));
                return; // 预检查经济
            }

            auto& db       = PLand::getInstance();
            auto& selector = LandSelector::getInstance();
            auto  data     = selector.getSelector(pl);
            if (!data) {
                return;
            }

            int const   length      = data->mPos.getDepth();
            int const   width       = data->mPos.getWidth();
            int const   height      = data->mPos.getHeight();
            auto const& squareRange = Config::cfg.land.bought.squareRange;
            if ((length < squareRange.min || width < squareRange.min) || // 长度和宽度必须大于最小值
                (length > squareRange.max || width > squareRange.max) || // 长度和宽度必须小于最大值
                (data->mDraw3D
                 && (height < squareRange.minHeight || height > mc_utils::GetDimensionMaxHeight(pl.getDimension()))
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

            auto lands = db.getLandAt(data->mPos.mMin_A, data->mPos.mMax_B, data->mDimid);
            if (!lands.empty()) {
                for (auto& land : lands) {
                    if (land->getLandID() == data->mBindLandData.lock()->getLandID()) continue; // 跳过自己
                    if (LandPos::isCollision(land->mPos, data->mPos)) {
                        mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地重叠，请重新选择"_trf(pl));
                        return;
                    }
                    if (!LandPos::isComplisWithMinSpacing(land->mPos, data->mPos)) {
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

            auto landPtr = data->mBindLandData;
            if (landPtr.lock()->_setLandPos(data->mPos)) {
                landPtr.lock()->mOriginalBuyPrice = discountedPrice; // 保存购买价格

                selector.completeAndRelease(pl);     // 释放数据
                db.refreshLandRange(landPtr.lock()); // 刷新领地范围

                mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "领地范围修改成功"_trf(pl));

                LandRangeChangeAfterEvent ev(pl, landPtr.lock(), data->mPos, needPay, refund);
                ll::event::EventBus::getInstance().publish(ev);

            } else mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "领地范围修改失败"_trf(pl));
        }
    );
    fm.appendButton("暂存订单"_trf(player), "textures/ui/recipe_book_icon"); // close
    fm.appendButton("放弃订单"_trf(player), "textures/ui/cancel", [](Player& pl) {
        LandSelector::getInstance().tryCancel(pl);
    });

    fm.sendTo(player);
}
void SelectorChangeYGui::impl(Player& player, std::string const& exception) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        return;
    }

    CustomForm fm(PLUGIN_NAME + ("| 确认Y轴范围"_trf(player)));

    fm.appendLabel("确认选区的Y轴范围\n您可以在此调节Y轴范围，如果不需要修改，请直接点击提交"_trf(player));

    fm.appendInput("start", "开始Y轴"_trf(player), "int", std::to_string(dataPtr->mPos.mMin_A.y));
    fm.appendInput("end", "结束Y轴"_trf(player), "int", std::to_string(dataPtr->mPos.mMax_B.y));

    fm.appendLabel(exception);

    fm.sendTo(player, [dataPtr](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }

        string start = std::get<string>(res->at("start"));
        string end   = std::get<string>(res->at("end"));

        if (!isNumber(start) || !isNumber(end) || isOutOfRange(start) || isOutOfRange(end)) {
            SelectorChangeYGui::impl(pl, "请输入正确的Y轴范围"_trf(pl));
            return;
        }

        try {
            int startY = std::stoi(start);
            int endY   = std::stoi(end);

            if (startY >= endY) {
                SelectorChangeYGui::impl(pl, "请输入正确的Y轴范围, 开始Y轴必须小于结束Y轴"_trf(pl));
                return;
            }

            dataPtr->mPos.mMin_A.y = startY;
            dataPtr->mPos.mMax_B.y = endY;
            dataPtr->mParticle.draw(pl, true); // force update cache

            mc_utils::sendText(pl, "Y轴范围已修改为 {} ~ {}"_trf(pl, startY, endY));
        } catch (...) {
            mc_utils::sendText<mc_utils::LogLevel::Fatal>(pl, "插件内部错误, 请联系开发者"_trf(pl));
            my_mod::MyMod::getInstance().getSelf().getLogger().error(
                "An exception is caught in {} and user {} enters data: {}, {}",
                __FUNCTION__,
                pl.getRealName(),
                start,
                end
            );
        }
    });
}


// 通用组件Gui
template <typename ParentForm>
void ChooseLandUtilGui::impl(Player& player, ChooseCallback const& callback, bool showShredLand) {
    auto fm = SimpleFormEx::create<ParentForm, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + ("| 选择领地"_trf(player)));
    fm.setContent("请选择一个领地"_trf(player));

    auto lands = PLand::getInstance().getLands(player.getUuid().asString(), showShredLand);
    for (auto& land : lands) {
        fm.appendButton(
            "{}\n维度: {} | ID: {}"_trf(player, land->getLandName(), land->getLandDimid(), land->getLandID()),
            "textures/ui/icon_recipe_nature",
            [callback, land](Player& pl) { callback(pl, land->getLandID()); }
        );
    }

    fm.sendTo(player);
}
template <typename ParentForm>
void ChoosePlayerUtilGui::impl(Player& player, ChoosePlayerCall const& callback) {
    auto fm = SimpleFormEx::create<LandMainGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + ("| 选择玩家"_trf(player)));

    ll::service::getLevel()->forEachPlayer([callback, &fm](Player& target) {
        if (target.isSimulatedPlayer()) {
            return true; // ignore
        }

        fm.appendButton(target.getRealName(), "", [callback, &target](Player& self) { callback(self, target); });
        return true;
    });

    fm.sendTo(player);
}
void EditStringUtilGui::impl(
    Player&                 player,
    string const&           title,        // 标题
    string const&           text,         // 提示
    string const&           defaultValue, // 默认值
    EditStringResult const& callback      // 回调
) {
    CustomForm fm(PLUGIN_NAME + title);
    fm.appendInput("str", text, "string", defaultValue);
    fm.sendTo(player, [callback](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }
        callback(pl, std::get<string>(res->at("str")));
    });
}
void FuzzySerarchUtilGui::impl(Player& player, std::vector<LandData_sptr> const& list, const CallBack& callback) {
    CustomForm fm;
    fm.setTitle(PLUGIN_NAME + " | 模糊搜索领地"_trf(player));
    fm.appendInput("name", "请输入领地名称"_trf(player), "string");
    fm.sendTo(player, [list, callback](Player& player, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        auto                       name = std::get<string>(res->at("name"));
        std::vector<LandData_sptr> filtered;
        for (auto const& ptr : list) {
            if (ptr->getLandName().find(name) != std::string::npos) {
                filtered.push_back(ptr);
            }
        }
        callback(player, filtered);
    });
}


// 领地主菜单
void LandMainGui::impl(Player& player) {
    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 领地菜单"_trf(player)));
    fm.setContent("欢迎使用 Pland 领地管理插件\n\n请选择一个功能"_trf(player));

    fm.appendButton("新建领地"_trf(player), "textures/ui/anvil_icon", [](Player& pl) {
        ChooseLandDimAndNewLand::impl(pl);
    });
    fm.appendButton("管理领地"_trf(player), "textures/ui/icon_spring", [](Player& pl) {
        ChooseLandUtilGui::impl<LandMainGui>(pl, LandManagerGui::impl);
    });
    fm.appendButton("领地传送"_trf(player), "textures/ui/icon_recipe_nature", [](Player& pl) {
        LandTeleportGui::impl(pl);
    });
    fm.appendButton("个人设置"_trf(player), "textures/ui/icon_recipe_nature", [](Player& pl) {
        EditPlayerSettingGui::impl(pl);
    });

    fm.appendButton("关闭"_trf(player), "textures/ui/cancel");
    fm.sendTo(player);
}


void EditPlayerSettingGui::impl(Player& player) {
    auto setting = PLand::getInstance().getPlayerSettings(player.getUuid().asString());

    CustomForm fm(PLUGIN_NAME + ("| 玩家设置"_trf(player)));

    fm.appendToggle("showEnterLandTitle", "是否显示进入领地提示"_trf(player), setting->showEnterLandTitle);
    fm.appendToggle("showBottomContinuedTip", "是否持续显示底部提示"_trf(player), setting->showBottomContinuedTip);

    fm.sendTo(player, [setting](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res) {
            return;
        }
        setting->showEnterLandTitle     = std::get<uint64_t>(res->at("showEnterLandTitle"));
        setting->showBottomContinuedTip = std::get<uint64_t>(res->at("showBottomContinuedTip"));
        mc_utils::sendText<mc_utils::LogLevel::Info>(pl, "设置已保存"_trf(pl));
    });
}


// 领地管理菜单
void LandManagerGui::impl(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地不存在"_trf(player));
        return;
    }

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 领地管理 [{}]"_trf(player, land->getLandID())));
    fm.setContent("领地: {}\n类型: {}\n大小: {}x{}x{} = {}\n范围: {}\n"_trf(
        player,
        land->getLandName(),
        land->is3DLand() ? "3D" : "2D",
        land->getLandPos().getDepth(),
        land->getLandPos().getWidth(),
        land->getLandPos().getHeight(),
        land->getLandPos().getVolume(),
        land->getLandPos().toString()
    ));

    fm.appendButton("编辑权限"_trf(player), "textures/ui/sidebar_icons/promotag", [land](Player& pl) {
        EditLandPermGui::impl(pl, land);
    });
    fm.appendButton("修改成员"_trf(player), "textures/ui/FriendsIcon", [land](Player& pl) {
        EditLandMemberGui::impl(pl, land);
    });
    fm.appendButton("修改领地名称"_trf(player), "textures/ui/book_edit_default", [land](Player& pl) {
        EditLandNameGui::impl(pl, land);
    });
    fm.appendButton("修改领地描述"_trf(player), "textures/ui/book_edit_default", [land](Player& pl) {
        EditLandDescGui::impl(pl, land);
    });
    fm.appendButton("传送到领地", "textures/ui/icon_recipe_nature", [id](Player& pl) { LandTeleportGui::run(pl, id); });
    fm.appendButton("领地过户"_trf(player), "textures/ui/sidebar_icons/my_characters", [land](Player& pl) {
        EditLandOwnerGui::impl(pl, land);
    });
    fm.appendButton("重新选区"_trf(player), "textures/ui/anvil_icon", [land](Player& pl) {
        ReSelectLandGui::impl(pl, land);
    });
    fm.appendButton("删除领地"_trf(player), "textures/ui/icon_trash", [land](Player& pl) {
        DeleteLandGui::impl(pl, land);
    });

    fm.sendTo(player);
}
void LandManagerGui::EditLandPermGui::impl(Player& player, LandData_sptr const& ptr) {
    CustomForm fm(PLUGIN_NAME + " | 编辑权限"_trf(player));

    auto& i18n = ll::i18n::getInstance();

    auto localeCode = GetPlayerLocaleCodeFromSettings(player);

    auto js = JSON::structTojson(ptr->getLandPermTableConst());
    for (auto& [k, v] : js.items()) {
        fm.appendToggle(k, (string)i18n.get(k, localeCode), v);
    }

    fm.sendTo(player, [ptr](Player& pl, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        auto& perm = ptr->getLandPermTable();
        auto  j    = JSON::structTojson(perm);
        for (auto const& [key, value] : j.items()) {
            bool const val = std::get<uint64_t>(res->at(key));
            j[key]         = val;
        }

        JSON::jsonToStructNoMerge(j, perm);

        mc_utils::sendText(pl, "权限表已更新"_trf(pl));
    });
}
void LandManagerGui::DeleteLandGui::impl(Player& player, LandData_sptr const& ptr) {
    int price = Calculate::calculateRefundsPrice(ptr->mOriginalBuyPrice, Config::cfg.land.refundRate);

    PlayerDeleteLandBeforeEvent ev(player, ptr->getLandID(), price);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    ModalForm fm(
        PLUGIN_NAME + " | 确认删除?"_trf(player),
        "您确定要删除领地 {} 吗?\n删除领地后，您将获得 {} 金币的退款。\n此操作不可逆,请谨慎操作!"_trf(
            player,
            ptr->getLandName(),
            price
        ),
        "确认"_trf(player),
        "返回"_trf(player)
    );
    fm.sendTo(player, [ptr, price](Player& pl, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        if (!(bool)res.value()) {
            LandManagerGui::impl(pl, ptr->getLandID());
            return;
        }

        auto& db  = PLand::getInstance();
        auto& eco = EconomySystem::getInstance();

        if (eco.add(pl, price)) {
            if (db.removeLand(ptr->getLandID())) {
                mc_utils::sendText(pl, "删除领地成功!"_trf(pl));
            } else mc_utils::sendText(pl, "删除领地失败!"_trf(pl));

            PlayerDeleteLandAfterEvent ev(pl, ptr->getLandID());
            ll::event::EventBus::getInstance().publish(ev);

        } else mc_utils::sendText(pl, "经济系统异常，操作失败"_trf(pl));
    });
}
void LandManagerGui::EditLandNameGui::impl(Player& player, LandData_sptr const& ptr) {
    EditStringUtilGui::impl(
        player,
        "修改领地名称"_trf(player),
        "请输入新的领地名称"_trf(player),
        ptr->getLandName(),
        [ptr](Player& pl, string result) {
            ptr->setLandName(result);
            mc_utils::sendText(pl, "领地名称已更新!"_trf(pl));
        }
    );
}
void LandManagerGui::EditLandDescGui::impl(Player& player, LandData_sptr const& ptr) {
    EditStringUtilGui::impl(
        player,
        "修改领地描述"_trf(player),
        "请输入新的领地描述"_trf(player),
        ptr->getLandDescribe(),
        [ptr](Player& pl, string result) {
            ptr->setLandDescribe(result);
            mc_utils::sendText(pl, "领地描述已更新!"_trf(pl));
        }
    );
}
void LandManagerGui::EditLandOwnerGui::impl(Player& player, LandData_sptr const& ptr) {
    ChoosePlayerUtilGui::impl(player, [ptr](Player& self, Player& target) {
        if (self.getUuid() == target.getUuid()) {
            mc_utils::sendText(self, "不能将领地转让给自己, 左手倒右手哦!"_trf(self));
            return;
        }

        LandOwnerChangeBeforeEvent ev(self, target, ptr->getLandID());
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm fm(
            PLUGIN_NAME + " | 确认转让?"_trf(self),
            "您确定要将领地转让给 {} 吗?\n转让后，您将失去此领地的权限。\n此操作不可逆,请谨慎操作!"_trf(
                self,
                target.getRealName()
            ),
            "确认"_trf(self),
            "返回"_trf(self)
        );
        fm.sendTo(self, [ptr, &target](Player& self, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }

            if (!(bool)res.value()) {
                LandManagerGui::impl(self, ptr->getLandID());
                return;
            }

            if (ptr->setLandOwner(target.getUuid().asString())) {
                mc_utils::sendText(self, "领地已转让给 {}"_trf(self, target.getRealName()));
                mc_utils::sendText(
                    target,
                    "您已成功接手来自 \"{}\" 的领地 \"{}\""_trf(self, self.getRealName(), ptr->getLandName())
                );

                LandOwnerChangeAfterEvent ev(self, target, ptr->getLandID());
                ll::event::EventBus::getInstance().publish(ev);
            } else {
                mc_utils::sendText<mc_utils::LogLevel::Error>(self, "领地转让失败!"_trf(self));
            }
        });
    });
}
void LandManagerGui::ReSelectLandGui::impl(Player& player, LandData_sptr const& ptr) {
    ModalForm fm(
        PLUGIN_NAME + " | 重新选区"_trf(player),
        "重新选区为完全重新选择领地的范围，非直接扩充/缩小现有领地范围。\n重新选择的价格计算方式为\"新范围价格 — 旧范围价值\"，是否继续？"_trf(
            player
        ),
        "确认"_trf(player),
        "返回"_trf(player)
    );
    fm.sendTo(player, [ptr](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        if (!(bool)res.value()) {
            LandManagerGui::impl(self, ptr->getLandID());
            return;
        }
        LandSelector::getInstance().tryReSelect(self, ptr);
    });
}


// 编辑领地成员
void EditLandMemberGui::impl(Player& player, LandData_sptr ptr) {
    auto fm = SimpleFormEx::create<LandManagerGui, BackButtonPos::Upper>(ptr->getLandID());

    fm.appendButton("添加成员"_trf(player), "textures/ui/color_plus", [ptr](Player& self) {
        AddMemberGui::impl(self, ptr);
    });

    auto& infos = ll::service::PlayerInfo::getInstance();
    for (auto& member : ptr->getLandMembers()) {
        auto i = infos.fromUuid(UUIDm::fromString(member));
        if (!i) {
            my_mod::MyMod::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
        }

        fm.appendButton(i.has_value() ? i->name : member, [member, ptr](Player& self) {
            RemoveMemberGui::impl(self, ptr, member);
        });
    }

    fm.sendTo(player);
}
void EditLandMemberGui::AddMemberGui::impl(Player& player, LandData_sptr ptr) {
    ChoosePlayerUtilGui::impl<EditLandMemberGui>(player, [ptr](Player& self, Player& target) {
        if (self.getUuid() == target.getUuid()) {
            mc_utils::sendText(self, "不能添加自己为领地成员哦!"_trf(self));
            return;
        }

        LandMemberChangeBeforeEvent ev(self, target.getUuid().asString(), ptr->getLandID(), true);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm fm(
            PLUGIN_NAME + " | 添加成员"_trf(self),
            "您确定要添加 {} 为领地成员吗?"_trf(self, target.getRealName()),
            "确认"_trf(self),
            "返回"_trf(self)
        );
        fm.sendTo(self, [ptr, &target](Player& self, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }

            if (!(bool)res.value()) {
                EditLandMemberGui::impl(self, ptr);
                return;
            }

            if (ptr->isLandMember(target.getUuid().asString())) {
                mc_utils::sendText(self, "该玩家已经是领地成员, 请不要重复添加哦!"_trf(self));
                return;
            }

            if (ptr->addLandMember(target.getUuid().asString())) {
                mc_utils::sendText(self, "添加成功!"_trf(self));

                LandMemberChangeAfterEvent ev(self, target.getUuid().asString(), ptr->getLandID(), true);
                ll::event::EventBus::getInstance().publish(ev);
            } else {
                mc_utils::sendText(self, "添加失败!"_trf(self));
            }
        });
    });
}
void EditLandMemberGui::RemoveMemberGui::impl(Player& player, LandData_sptr ptr, UUIDs member) {
    LandMemberChangeBeforeEvent ev(player, member, ptr->getLandID(), false);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto info = ll::service::PlayerInfo::getInstance().fromUuid(UUIDm::fromString(member));
    if (!info) {
        my_mod::MyMod::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
    }

    ModalForm fm(
        PLUGIN_NAME + " | 移除成员"_trf(player),
        "您确定要移除成员 \"{}\" 吗?"_trf(player, info.has_value() ? info->name : member),
        "确认"_trf(player),
        "返回"_trf(player)
    );
    fm.sendTo(player, [ptr, member](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        if (!(bool)res.value()) {
            EditLandMemberGui::impl(self, ptr);
            return;
        }

        if (ptr->removeLandMember(member)) {
            mc_utils::sendText(self, "移除成功!"_trf(self));

            LandMemberChangeAfterEvent ev(self, member, ptr->getLandID(), false);
            ll::event::EventBus::getInstance().publish(ev);
        } else {
            mc_utils::sendText(self, "移除失败!"_trf(self));
        }
    });
}


// 领地传送GUI
void LandTeleportGui::impl(Player& player) { ChooseLandUtilGui::impl<LandMainGui>(player, LandTeleportGui::run, true); }
void LandTeleportGui::run(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "领地不存在"_trf(player));
        return;
    }

    if (land->mTeleportPos.isZero()) {
        SafeTeleport::getInstance().teleportTo(player, land->mPos.mMin_A, land->getLandDimid());
        return;
    }

    player.teleport(land->mTeleportPos, land->getLandDimid());
}


// 管理员GUI
void LandOPManagerGui::impl(Player& player) {
    auto* db = &PLand::getInstance();
    if (!db->isOperator(player.getUuid().asString())) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(player, "无权限访问此表单"_trf(player));
        return;
    }

    auto fm = SimpleFormEx::create();

    fm.setTitle(PLUGIN_NAME + " | 领地管理"_trf(player));
    fm.setContent("请选择您要进行的操作"_trf(player));

    fm.appendButton("管理脚下领地"_trf(player), "textures/ui/free_download", [db](Player& self) {
        auto lands = db->getLandAt(self.getPosition(), self.getDimensionId());
        if (!lands) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(self, "您当前所处位置没有领地"_trf(self));
            return;
        }
        LandManagerGui::impl(self, lands->getLandID());
    });
    fm.appendButton("管理玩家领地"_trf(player), "textures/ui/FriendsIcon", [](Player& self) {
        IChoosePlayerFromDB::impl(self, ManageLandWithPlayer::impl);
    });
    fm.appendButton("管理指定领地"_trf(player), "textures/ui/magnifyingGlass", [](Player& self) {
        IChooseLand::impl(self, PLand::getInstance().getLands());
    });

    fm.sendTo(player);
}
void LandOPManagerGui::ManageLandWithPlayer::impl(Player& player, UUIDs const& targetPlayer) {
    IChooseLand::impl(player, PLand::getInstance().getLands(targetPlayer));
}


void LandOPManagerGui::IChoosePlayerFromDB::impl(Player& player, ChoosePlayerCall callback) {
    auto fm = SimpleFormEx::create<LandOPManagerGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + " | 玩家列表"_trf(player));
    fm.setContent("请选择您要管理的玩家"_trf(player));

    auto const& db    = PLand::getInstance();
    auto const& infos = ll::service::PlayerInfo::getInstance();
    auto const  lands = db.getLands();

    std::unordered_set<UUIDs> filtered; // 防止重复
    for (auto const& ptr : lands) {
        if (filtered.contains(ptr->getLandOwner())) {
            continue;
        }
        filtered.insert(ptr->getLandOwner());
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getLandOwner()));

        fm.appendButton(info.has_value() ? info->name : ptr->getLandOwner(), [ptr, callback](Player& self) {
            callback(self, ptr->getLandOwner());
        });
    }

    fm.sendTo(player);
}


void LandOPManagerGui::IChooseLand::impl(Player& player, std::vector<LandData_sptr> const& lands) {
    auto fm = SimpleFormEx::create<LandOPManagerGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + " | 领地列表"_trf(player));
    fm.setContent("请选择您要管理的领地"_trf(player));

    fm.appendButton("模糊搜索领地", "textures/ui/magnifyingGlass", [lands](Player& player) {
        FuzzySerarchUtilGui::impl(player, lands, LandOPManagerGui::IChooseLand::impl);
    });

    auto const& infos = ll::service::PlayerInfo::getInstance();
    for (auto const& ptr : lands) {
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getLandOwner()));
        fm.appendButton(
            "{}\nID: {}  玩家: {}"_trf(
                player,
                ptr->getLandName(),
                ptr->getLandID(),
                info.has_value() ? info->name : ptr->getLandOwner()
            ),
            [ptr](Player& self) { LandManagerGui::impl(self, ptr->getLandID()); }
        );
    }

    fm.sendTo(player);
}


} // namespace land