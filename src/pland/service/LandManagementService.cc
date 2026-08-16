#include "LandManagementService.h"
#include "LandHierarchyService.h"
#include "LandPriceService.h"

#include "ll/api/coro/CoroTask.h"
#include "pland/events/domain/LandResizedEvent.h"
#include "pland/events/domain/MemberChangedEvent.h"
#include "pland/events/domain/OwnerChangedEvent.h"
#include "pland/events/economy/LandRefundFailedEvent.h"
#include "pland/events/player/PlayerApplyLandRangeChangeEvent.h"
#include "pland/events/player/PlayerBuyLandEvent.h"
#include "pland/events/player/PlayerChangeLandMemberEvent.h"
#include "pland/events/player/PlayerChangeLandNameEvent.h"
#include "pland/events/player/PlayerDeleteLandEvent.h"
#include "pland/events/player/PlayerRequestChangeLandRangeEvent.h"
#include "pland/events/player/PlayerRequestCreateLandEvent.h"
#include "pland/events/player/PlayerTransferLandEvent.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/LandResizeSettlement.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/land/validator/LandCreateValidator.h"
#include "pland/land/validator/StringValidator.h"
#include "pland/selector/land/LandResizeSelector.h"
#include "pland/selector/land/OrdinaryLandCreateSelector.h"
#include "pland/selector/land/SubLandCreateSelector.h"
#include "pland/service/SelectionService.h"


#include "ll/api/Expected.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/i18n/I18n.h"
#include <ll/api/event/EventBus.h>


#include <chrono>
#include <mc/world/actor/player/Player.h>
#include <memory>


namespace land {
namespace service {

struct LandManagementService::Impl {
    LandRegistry&         mRegistry;
    SelectionService&     mSelectionService;
    LandHierarchyService& mHierarchyService;
    LandPriceService&     mPriceService;

    [[deprecated]] ll::event::ListenerPtr mLegacyOwnerMigrationListener;
};

LandManagementService::LandManagementService(
    LandRegistry&         registry,
    SelectionService&     selectionService,
    LandHierarchyService& hierarchyService,
    LandPriceService&     priceService
)
: impl(std::make_unique<Impl>(registry, selectionService, hierarchyService, priceService)) {
    // TODO: 移除此监听器
    // 虽然 v0.18.0 移除了对 iLand 数据转换的支持，但依然可能有残存的数据未转换
    // 因此这里仅作临时转换处理，待基本无旧数据后，移除此监听器
    impl->mLegacyOwnerMigrationListener =
        ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerJoinEvent>(
            [this](ll::event::PlayerJoinEvent const& ev) {
                auto& player = ev.self();
                if (player.isSimulatedPlayer()) return;

                auto xuid = player.getXuid();
                auto uuid = player.getUuid();

                auto lands = impl->mRegistry.getLandsWhere([&xuid](std::shared_ptr<Land> const& land) {
                    return land->isOwnerDataIsXUID() && land->getRawOwner() == xuid;
                });
                if (!lands.empty()) {
                    for (auto& land : lands) {
                        land->migrateOwner(uuid);
                    }
                }
            }
        );
}
LandManagementService::~LandManagementService() {
    ll::event::EventBus::getInstance().removeListener(impl->mLegacyOwnerMigrationListener);
}


ll::Expected<> LandManagementService::requestCreateOrdinaryLand(Player& player, bool is3D) const {
    if (!Config::ensureDimensionAllowed(player.getDimensionId())) {
        return ll::makeStringError("你所在的维度无法购买领地"_trl(player.getLocaleCode()));
    }
    if (!Config::ensureOrdinaryLandEnabled(is3D)) {
        return ll::makeStringError("当前领地类型未启用，请联系管理员"_trl(player.getLocaleCode()));
    }

    ll::event::EventBus::getInstance().publish(event::PlayerRequestCreateLandEvent{player, LandType::Ordinary});

    auto selector = std::make_unique<OrdinaryLandCreateSelector>(player, is3D);
    return impl->mSelectionService.beginSelection(player, std::move(selector));
}

ll::Expected<> LandManagementService::requestCreateSubLand(Player& player) {
    auto land = impl->mRegistry.getLandAt(player.getPosition(), player.getDimensionId());
    if (!land) {
        return ll::makeStringError("操作失败, 当前位置没有领地"_trl(player.getLocaleCode()));
    }
    return requestCreateSubLand(player, land);
}
ll::Expected<> LandManagementService::requestCreateSubLand(Player& player, std::shared_ptr<Land> const& land) const {
    if (!Config::ensureSubLandFeatureEnabled()) {
        return ll::makeStringError("子领地功能未启用"_trl(player.getLocaleCode()));
    }
    if (!Config::ensureDimensionAllowed(player.getDimensionId())) {
        return ll::makeStringError("你所在的维度无法购买领地"_trl(player.getLocaleCode()));
    }
    if (!land->isOwner(player.getUuid())) {
        return ll::makeStringError("操作失败, 您不是当前领地主人"_trl(player.getLocaleCode()));
    }
    if (land->isLeased()) {
        return ll::makeStringError("租赁领地暂不支持创建子领地"_trl(player.getLocaleCode()));
    }
    if (!land->canCreateSubLand()) {
        return ll::makeStringError("操作失败，当前领地无法继续创建子领地"_trl(player.getLocaleCode()));
    }

    ll::event::EventBus::getInstance().publish(event::PlayerRequestCreateLandEvent{player, LandType::Sub});

    auto selector = std::make_unique<SubLandCreateSelector>(player, land);
    return impl->mSelectionService.beginSelection(player, std::move(selector));
}

ll::Expected<std::shared_ptr<Land>>
LandManagementService::buyLand(Player& player, OrdinaryLandCreateSelector* selector, int64_t money) {
    assert(selector);
    auto event = event::PlayerBuyLandBeforeEvent{player, money, LandType::Ordinary};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }

    auto exp = _payMoneyAndCreateOrdinaryLand(player, selector, money);
    if (!exp) {
        return exp;
    }
    impl->mSelectionService.endSelection(player);
    ll::event::EventBus::getInstance().publish(event::PlayerBuyLandAfterEvent{player, exp.value(), money});
    return exp.value();
}

ll::Expected<std::shared_ptr<Land>>
LandManagementService::buyLand(Player& player, SubLandCreateSelector* selector, int64_t money) {
    assert(selector != nullptr);
    auto event = event::PlayerBuyLandBeforeEvent{player, money, LandType::Sub};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }
    auto exp = _payMoneyAndCreateSubLand(player, selector, money);
    if (!exp) {
        return exp;
    }
    impl->mSelectionService.endSelection(player);
    ll::event::EventBus::getInstance().publish(event::PlayerBuyLandAfterEvent{player, exp.value(), money});
    return exp.value();
}

ll::Expected<> LandManagementService::handleChangeRange(
    Player&                     player,
    LandResizeSelector*         selector,
    LandResizeSettlement const& settlement
) {
    auto land = selector->getLand();
    if (!land) {
        return ll::makeStringError("获取领地失败"_trl(player.getLocaleCode()));
    }
    if (!land->isOrdinaryLand()) {
        return ll::makeStringError("操作失败，当前领地无法调整大小"_trl(player.getLocaleCode()));
    }

    auto range = selector->newLandAABB();
    assert(range.has_value());

    auto event = event::PlayerApplyLandRangeChangeBeforeEvent{player, land, *range, settlement};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }

    auto expected = _ensureChangeRangelegal(land, *range, player.getLocaleCode());
    if (!expected) return expected;

    expected = _processResizeSettlement(player, settlement);
    if (!expected) return expected;

    expected = _applyRangeChange(player, land, *range);
    if (!expected) return expected;

    land->setOriginalBuyPrice(settlement.newTotalPrice);
    impl->mSelectionService.endSelection(player);

    ll::event::EventBus::getInstance().publish(
        event::PlayerApplyLandRangeChangeAfterEvent{player, land, *range, settlement}
    );
    return {};
}


ll::Expected<> LandManagementService::ensurePlayerLandCountLimit(mce::UUID const& uuid) const {
    return LandCreateValidator::ensurePlayerLandCountNotExceeded(impl->mRegistry, uuid);
}
ll::Expected<>
LandManagementService::setLandTeleportPos(Player& player, std::shared_ptr<Land> const& land, Vec3 point) {
    if (!land->isOwner(player.getUuid()) && !impl->mRegistry.isOperator(player.getUuid())) {
        return ll::makeStringError("操作失败，您不是领地主人"_trl(player.getLocaleCode()));
    }
    if (!land->getAABB().hasPos(point)) {
        return ll::makeStringError("设置传送点失败，传送点不在领地范围内"_trl(player.getLocaleCode()));
    }
    if (!land->setTeleportPos(LandPos::make(point))) {
        return ll::makeStringError("设置传送点失败"_trl(player.getLocaleCode()));
    }
    return {};
}
ll::Expected<> LandManagementService::deleteLand(Player& player, std::shared_ptr<Land> ptr, DeletePolicy policy) {
    if (auto res = _ensureLandWithDeletePolicy(player, ptr, policy); !res) {
        return res;
    }

    auto event = event::PlayerDeleteLandBeforeEvent{player, ptr};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }

    auto expected = _processDeleteLand(player, ptr, policy);
    if (!expected) return expected;

    // 仅非租赁领地执行退款逻辑
    if (!ptr->isLeased()) {
        if (auto refund = _processLandRefund(player, ptr, policy != DeletePolicy::Recursive); !refund) {
            return refund;
        }
    }
    return {};
}
ll::Expected<> LandManagementService::setLandName(Player& player, std::shared_ptr<Land> const& land, std::string name) {
    auto const& rule = ConfigProvider::getConstraintsConfig().nameRule;

    auto result = StringValidator::validate(
        name,
        "领地名称"_trl(player.getLocaleCode()),
        rule.minLen,
        rule.maxLen,
        rule.allowNewline,
        player.getLocaleCode()
    );
    if (!result) return result;

    auto event = event::PlayerChangeLandNameBeforeEvent{player, land, name};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }

    land->setName(name);

    ll::event::EventBus::getInstance().publish(event::PlayerChangeLandNameAfterEvent{player, land, name});
    return {};
}

ll::Expected<> LandManagementService::transferLand(Player& player, std::shared_ptr<Land> const& land, Player& target) {
    return transferLand(player, land, target.getUuid());
}

ll::Expected<>
LandManagementService::transferLand(Player& player, std::shared_ptr<Land> const& land, mce::UUID const& target) {
    auto const actorUuid  = player.getUuid();
    auto const isOperator = impl->mRegistry.isOperator(actorUuid);
    if (!isOperator && actorUuid == target) {
        return ll::makeStringError("不能将领地转让给自己, 左手倒右手哦!"_trl(player.getLocaleCode()));
    }

    if (auto expected = ensurePlayerLandCountLimit(target); !expected) {
        if (expected.error().isA<LandCreateValidator::ValidateError>()) {
            auto& error = expected.error().as<LandCreateValidator::ValidateError>();
            return ll::makeStringError(error.translateError(player.getLocaleCode()));
        }
        return expected;
    }

    auto event = event::PlayerTransferLandBeforeEvent{player, land, target};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }

    land->setOwner(target);

    ll::event::EventBus::getInstance().publish(event::PlayerTransferLandAfterEvent{player, land, target});
    return {};
}

ll::Expected<> LandManagementService::requestChangeRange(Player& player, std::shared_ptr<Land> const& land) {
    if (!land->isOrdinaryLand()) {
        return ll::makeStringError("操作失败, 仅支持普通领地调整范围"_trl(player.getLocaleCode()));
    }
    if (land->isLeased()) {
        return ll::makeStringError("租赁领地暂不支持调整范围"_trl(player.getLocaleCode()));
    }

    auto event = event::PlayerRequestChangeLandRangeBeforeEvent{player, land};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }

    auto selector = std::make_unique<LandResizeSelector>(player, land);
    auto expected = impl->mSelectionService.beginSelection(player, std::move(selector));
    if (!expected) {
        return expected;
    }

    ll::event::EventBus::getInstance().publish(event::PlayerRequestChangeLandRangeAfterEvent{player, land});
    return {};
}

ll::Expected<>
LandManagementService::addMember(Player& player, std::shared_ptr<Land> const& land, mce::UUID const& target) {
    return _playerChangeMember(player, land, target, true);
}

ll::Expected<>
LandManagementService::removeMember(Player& player, std::shared_ptr<Land> const& land, mce::UUID const& target) {
    return _playerChangeMember(player, land, target, false);
}

ll::Expected<> LandManagementService::_playerChangeMember(
    Player&                      player,
    std::shared_ptr<Land> const& land,
    mce::UUID const&             target,
    bool                         isAdd
) {
    if (isAdd) {
        if (land->isOwner(target)) {
            return ll::makeStringError("操作失败，领地主人不能被添加为成员"_trl(player.getLocaleCode()));
        }
        if (land->isMember(target)) {
            return ll::makeStringError("操作失败，该玩家已经是领地成员"_trl(player.getLocaleCode()));
        }
    } else {
        if (!land->isMember(target)) {
            return ll::makeStringError("操作失败，该玩家不是领地成员"_trl(player.getLocaleCode()));
        }
    }

    auto event = event::PlayerChangeLandMemberBeforeEvent{player, land, target, isAdd};
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return ll::makeStringError("操作失败，请求被取消"_trl(player.getLocaleCode()));
    }

    auto result = isAdd ? land->addLandMember(target) : land->removeLandMember(target);

    if (result) {
        ll::event::EventBus::getInstance().publish(
            event::PlayerChangeLandMemberAfterEvent{player, land, target, isAdd}
        );
    }
    return {};
}


ll::Expected<> LandManagementService::_ensureChangeRangelegal(
    std::shared_ptr<Land> const&    land,
    LandAABB const&                 newRange,
    std::optional<std::string_view> localeCode
) {
    if (auto res = LandCreateValidator::validateChangeLandRange(impl->mRegistry, land, newRange); !res) {
        if (res.error().isA<LandCreateValidator::ValidateError>()) {
            auto& error = res.error().as<LandCreateValidator::ValidateError>();
            return ll::makeStringError(error.translateError(localeCode.value_or(ll::i18n::getDefaultLocaleCode()).data()
            ));
        }
        return res;
    }
    return {};
}

ll::Expected<>
LandManagementService::_applyRangeChange(Player& player, std::shared_ptr<Land> const& land, LandAABB const& newRange) {
    if (auto res = _ensureChangeRangelegal(land, newRange, player.getLocaleCode()); !res) {
        return res;
    }
    land->_setAABB(newRange);
    impl->mRegistry.refreshLandRange(land);
    ll::event::EventBus::getInstance().publish(event::LandResizedEvent{land, newRange});
    return {};
}

ll::Expected<std::shared_ptr<Land>> LandManagementService::_payMoneyAndCreateOrdinaryLand(
    Player&                     player,
    OrdinaryLandCreateSelector* selector,
    int64_t                     money
) {
    assert(selector != nullptr);
    auto& economy = EconomySystem::getInstance();
    if (!economy.reduceChecked(player.getUuid(), money)) {
        return ll::makeStringError("您的余额不足，无法购买"_trl(player.getLocaleCode()));
    }
    auto land = selector->newLand();
    land->setOriginalBuyPrice(money);

    auto exp = _addOrdinaryLand(player, land);
    if (!exp) {
        (void)economy->add(player.getUuid(), money);
        return ll::makeStringError(exp.error().message());
    }
    return land;
}

ll::Expected<> LandManagementService::_addOrdinaryLand(Player& player, std::shared_ptr<Land> ptr) {
    assert(ptr != nullptr);
    if (auto res = LandCreateValidator::validateCreateOrdinaryLand(impl->mRegistry, player, ptr); !res) {
        if (res.error().isA<LandCreateValidator::ValidateError>()) {
            auto& error = res.error().as<LandCreateValidator::ValidateError>();
            return ll::makeStringError(error.translateError(player.getLocaleCode()));
        }
        return res;
    }
    return impl->mRegistry.addOrdinaryLand(ptr);
}

ll::Expected<std::shared_ptr<Land>>
LandManagementService::_payMoneyAndCreateSubLand(Player& player, SubLandCreateSelector* selector, int64_t money) {
    assert(selector != nullptr);
    auto& economy = EconomySystem::getInstance();
    if (!economy.reduceChecked(player.getUuid(), money)) {
        return ll::makeStringError("您的余额不足，无法购买"_trl(player.getLocaleCode()));
    }
    auto parent = selector->getParentLand();
    assert(parent != nullptr);

    auto sub = selector->newSubLand();
    sub->setOriginalBuyPrice(money);

    auto exp = _ensureAndAttachSubLand(player, parent, sub);
    if (!exp) {
        (void)economy->add(player.getUuid(), money);
        return ll::makeStringError(exp.error().message());
    }
    return sub;
}

ll::Expected<> LandManagementService::_ensureAndAttachSubLand(
    Player&               player,
    std::shared_ptr<Land> parent,
    std::shared_ptr<Land> sub
) {
    auto expected = LandCreateValidator::validateCreateSubLand(
        player,
        parent,
        sub->getAABB(),
        impl->mRegistry,
        impl->mHierarchyService
    );
    if (expected) {
        // 验证通过，进行附加操作
        expected = impl->mHierarchyService.attachSubLand(parent, sub);
    }

    // 检查错误
    if (!expected) {
        if (expected.error().isA<LandCreateValidator::ValidateError>()) {
            auto& error = expected.error().as<LandCreateValidator::ValidateError>();
            return ll::makeStringError(error.translateError(player.getLocaleCode()));
        }
        return expected;
    }
    return {};
}

ll::Expected<> LandManagementService::_processResizeSettlement(Player& player, LandResizeSettlement const& settlement) {
    auto& economy = EconomySystem::getInstance();
    switch (settlement.type) {
    case LandResizeSettlement::Type::Pay:
        if (!economy.reduceChecked(player.getUuid(), settlement.amount)) {
            return ll::makeStringError("您的余额不足，无法购买"_trl(player.getLocaleCode()));
        }
        break;
    case LandResizeSettlement::Type::Refund:
        if (!economy->add(player.getUuid(), settlement.amount)) {
            return ll::makeStringError("经济系统异常,退还差价失败"_trl(player.getLocaleCode()));
        }
        break;
    default:;
    }
    return {};
}
ll::Expected<> LandManagementService::_ensureLandWithDeletePolicy(
    Player&                      player,
    std::shared_ptr<Land> const& land,
    DeletePolicy                 policy
) {
    switch (policy) {
    case DeletePolicy::CurrentOnly:
        if (land->isOrdinaryLand() || land->isSubLand()) {
            return {};
        }
        break;
    case DeletePolicy::Recursive:
        if (land->isParentLand() || land->isMixLand()) {
            return {};
        }
        break;
    case DeletePolicy::PromoteChildren:
        if (land->isParentLand()) {
            return {};
        }
        break;
    case DeletePolicy::TransferChildren:
        if (land->isMixLand()) {
            return {};
        }
        break;
    }
    return ll::makeStringError("该领地无法删除, 指定的删除策略不合法"_trl(player.getLocaleCode()));
}

ll::Expected<>
LandManagementService::_processDeleteLand(Player& player, std::shared_ptr<Land> const& land, DeletePolicy policy) {
    std::unordered_set<std::shared_ptr<Land>> influence;

    ll::Expected<> expected;
    switch (policy) {
    case DeletePolicy::CurrentOnly:
        influence.insert(land);
        expected = land->isOrdinaryLand() ? impl->mRegistry.removeOrdinaryLand(land)
                                          : impl->mHierarchyService.deleteSubLand(land);
        break;
    case DeletePolicy::Recursive:
        influence = impl->mHierarchyService.getSelfAndDescendants(land);
        expected  = impl->mHierarchyService.deleteLandRecursive(land);
        break;
    case DeletePolicy::PromoteChildren:
        influence.insert(land);
        expected = impl->mHierarchyService.deleteParentLandAndPromoteChildren(land);
        break;
    case DeletePolicy::TransferChildren:
        influence.insert(land);
        expected = impl->mHierarchyService.deleteMixLandAndTransferChildren(land);
        break;
    }
    if (expected) {
        for (const auto& ld : influence) {
            ll::event::EventBus::getInstance().publish(event::PlayerDeleteLandAfterEvent(player, ld));
        }
    }
    return expected;
}

ll::Expected<>
LandManagementService::_processLandRefund(Player& player, std::shared_ptr<Land> const& land, bool isSingle) {
    if (land->isLeased()) {
        return {};
    }
    auto price =
        isSingle ? impl->mPriceService.getRefundAmount(land) : impl->mPriceService.getRefundAmountRecursively(land);
    auto& economy = EconomySystem::getInstance();
    if (!economy->add(player.getUuid(), price)) {
        ll::event::EventBus::getInstance().publish(event::LandRefundFailedEvent{land, player.getUuid(), price});
        return ll::makeStringError("经济系统异常,退还差价失败"_trl(player.getLocaleCode()));
    }
    return {};
}


} // namespace service
} // namespace land
