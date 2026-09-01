#include "SelectionService.h"
#include "pland/selector/AbstractSelector.h"
#include "pland/selector/SelectorManager.h"

#include "ll/api/Expected.h"

#include "mc/world/actor/player/Player.h"
namespace land::service {

struct SelectionService::Impl {
    SelectorManager& mManager;
};

SelectionService::SelectionService(SelectorManager& manager) : impl(std::make_unique<Impl>(manager)) {}
SelectionService::~SelectionService() = default;

bool SelectionService::hasActiveSelection(Player& player) const { return impl->mManager.hasSelector(player); }

ll::Expected<> SelectionService::beginSelection(Player& player, std::unique_ptr<AbstractSelector> selector) {
    if (hasActiveSelection(player)) {
        return ll::makeStringError("选区开启失败，当前存在未完成的选区任务"_trl(player.getLocaleCode()));
    }
    if (impl->mManager.startSelection(std::move(selector))) {
        return {};
    }
    return ll::makeStringError("选区开启失败，未知错误"_trl(player.getLocaleCode()));
}

void SelectionService::endSelection(Player& player) {
    if (hasActiveSelection(player)) {
        impl->mManager.stopSelection(player);
    }
}

AbstractSelector* SelectionService::tryGetSelection(Player& player) const {
    if (hasActiveSelection(player)) {
        return impl->mManager.getSelector(player);
    }
    return nullptr;
}


} // namespace land::service