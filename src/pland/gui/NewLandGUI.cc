#include "NewLandGUI.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"

#include "mc/world/actor/player/Player.h"

#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/selector/ABSelector.h"
#include "pland/selector/SelectorManager.h"
#include "pland/selector/land/SubLandCreateSelector.h"
#include "pland/service/LandManagementService.h"
#include "pland/service/ServiceLocator.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/McUtils.h"


#include <string>


using namespace ll::form;


namespace land::gui {


void NewLandGUI::sendChooseLandDim(Player& player) {
    auto localeCode = player.getLocaleCode();

    ModalForm(
        ("[PLand] | 选择领地维度"_trl(localeCode)),
        "请选择领地维度\n\n2D: 领地拥有整个Y轴\n3D: 自行选择Y轴范围"_trl(localeCode),
        "2D", // true
        "3D"  // false
    )
        .sendTo(player, [](Player& pl, ModalFormResult res, FormCancelReason) {
            if (!res.has_value()) {
                return;
            }

            bool is3D = !((bool)res.value());

            auto& service = PLand::getInstance().getServiceLocator().getLandManagementService();

            auto expected = service.requestCreateOrdinaryLand(pl, is3D);
            if (!expected) {
                feedback_utils::sendError(pl, expected.error());
                return;
            }

            feedback_utils::sendText(
                pl,
                "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trl(
                    pl.getLocaleCode(),
                    ConfigProvider::getSelectionConfig().alias
                )
            );
        });
}


void NewLandGUI::sendConfirmPrecinctsYRange(Player& player, std::string const& exception) {
    auto selector = land::PLand::getInstance().getSelectorManager()->getSelector(player);
    if (!selector) {
        return;
    }
    auto localeCode = player.getLocaleCode();

    CustomForm fm(("[PLand] | 确认Y轴范围"_trl(localeCode)));

    fm.appendLabel("确认选区的Y轴范围\n您可以在此调节Y轴范围，如果不需要修改，请直接点击提交"_trl(localeCode));

    SubLandCreateSelector* subSelector = nullptr;
    std::shared_ptr<Land>  parentLand  = nullptr;
    if (subSelector = selector->as<SubLandCreateSelector>(); subSelector) {
        if (parentLand = subSelector->getParentLand(); parentLand) {
            auto& aabb = parentLand->getAABB();
            fm.appendLabel(
                "当前为子领地模式，子领地的Y轴范围不能超过父领地。\n父领地Y轴范围: {} ~ {}"_trl(
                    localeCode,
                    aabb.min.y,
                    aabb.max.y
                )
            );
        }
    }

    ABSelector* abSelector{nullptr};
    if (abSelector = selector->as<ABSelector>(); !abSelector) {
        // TODO: handle thirdparty AbstractSelector
        return;
    }

    fm.appendInput("start", "开始Y轴"_trl(localeCode), "int", std::to_string(abSelector->getPointA()->y));
    fm.appendInput("end", "结束Y轴"_trl(localeCode), "int", std::to_string(abSelector->getPointB()->y));

    fm.appendLabel(exception);

    fm.sendTo(player, [abSelector, subSelector, parentLand](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }

        std::string start = std::get<std::string>(res->at("start"));
        std::string end   = std::get<std::string>(res->at("end"));

        auto localeCode = pl.getLocaleCode();
        try {
            int64_t startY = std::stoll(start);
            int64_t endY   = std::stoll(end);
            if (startY >= INT32_MAX || startY <= INT32_MIN || endY >= INT32_MAX || endY <= INT32_MIN) {
                sendConfirmPrecinctsYRange(pl, "数值过大，请输入正确的Y轴范围"_trl(localeCode));
                return;
            }

            if (startY >= endY) {
                sendConfirmPrecinctsYRange(pl, "请输入正确的Y轴范围, 开始Y轴必须小于结束Y轴"_trl(localeCode));
                return;
            }

            if (subSelector) {
                if (!subSelector || !parentLand) {
                    throw std::runtime_error("subSelector or parentLand is nullptr");
                }

                auto& aabb = parentLand->getAABB();
                if (startY < aabb.min.y || endY > aabb.max.y) {
                    sendConfirmPrecinctsYRange(
                        pl,
                        "请输入正确的Y轴范围, 子领地的Y轴范围不能超过父领地"_trl(localeCode)
                    );
                    return;
                }
            }

            abSelector->setYRange((int)startY, (int)endY);
            abSelector->onPointConfirmed();
        } catch (...) {
            sendConfirmPrecinctsYRange(pl, "处理失败,请输入正确的Y轴范围"_trl(localeCode));
        }
    });
}


} // namespace land::gui
