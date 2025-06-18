#include "pland/LandSelector.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/deps/core/math/Color.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/item/ItemStack.h"
#include "mod/ModEntry.h"
#include "pland/Config.h"
#include "pland/DrawHandleManager.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/utils/Date.h"
#include "pland/utils/McUtils.h"
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>


namespace land {


Selector::~Selector() {
    if (mDrawedAABBGeoId) {
        DrawHandleManager::getInstance().getOrCreateHandle(*mPlayer)->remove(mDrawedAABBGeoId);
    }
}

Selector::Selector(Player& player, int dimid, bool is3D, Type type)
: mType(type),
  mPlayer(&player),
  mDimensionId(dimid),
  mIs3D(is3D),
  mDrawedAABBGeoId{} {}

std::unique_ptr<Selector> Selector::createDefault(Player& player, int dimid, bool is3D) {
    return std::make_unique<Selector>(player, dimid, is3D);
}

Player& Selector::getPlayer() const { return *mPlayer; }

int Selector::getDimensionId() const { return mDimensionId; }

bool Selector::is3D() const { return mIs3D; }

std::optional<BlockPos> Selector::getPointA() const { return mPointA; }

std::optional<BlockPos> Selector::getPointB() const { return mPointB; }

std::optional<LandAABB> Selector::getAABB() const {
    if (mPointA.has_value() && mPointB.has_value()) {
        auto aabb = LandAABB::make(mPointA.value(), mPointB.value());
        aabb.fix();
        return aabb;
    }
    return std::nullopt;
}

LandData_sptr Selector::newLandData() const {
    if (auto aabb = getAABB(); aabb) {
        return LandData::make(aabb.value(), mDimensionId, mIs3D, mPlayer->getUuid().asString());
    }
    return nullptr;
}

Selector::Type Selector::getType() const { return mType; }


bool Selector::isSelectorTool(ItemStack const& item) const { return item.getTypeName() == Config::cfg.selector.tool; }

bool Selector::canSelect() const { return canSelectPointA() || canSelectPointB(); }

bool Selector::canSelectPointA() const { return !mPointA.has_value(); }

bool Selector::canSelectPointB() const { return !mPointB.has_value(); }

void Selector::selectPointA(BlockPos const& pos) { this->mPointA = pos; }

void Selector::selectPointB(BlockPos const& pos) {
    this->mPointB = pos;
    fixAABBMinMax();
}

void Selector::fixAABBMinMax() {
    if (!mPointA.has_value() || !mPointB.has_value()) {
        return;
    }

    if (mPointA->x > mPointB->x) std::swap(mPointA->x, mPointB->x);
    if (mPointA->y > mPointB->y) std::swap(mPointA->y, mPointB->y);
    if (mPointA->z > mPointB->z) std::swap(mPointA->z, mPointB->z);
}

void Selector::drawAABB() {
    if (auto aabb = getAABB(); aabb && !mDrawedAABBGeoId) {
        mDrawedAABBGeoId = DrawHandleManager::getInstance().getOrCreateHandle(*mPlayer)->draw(
            aabb.value(),
            mDimensionId,
            mce::Color::WHITE()
        );
    }
}

void Selector::onABSelected() {
    if (mDrawedAABBGeoId) {
        return;
    }

    auto& player = getPlayer();

    if (is3D()) {
        // 3DLand
        SelectorChangeYGui::impl(player); // 发送GUI
    } else {
        // 2DLand
        if (auto dim = player.getLevel().getDimension(mDimensionId).lock(); dim) {
            this->mPointA.value().y = mc_utils::GetDimensionMinHeight(*dim);
            this->mPointB.value().y = mc_utils::GetDimensionMaxHeight(*dim);

            onFixesY(); // Y 轴已修正
        } else {
            mc_utils::sendText<mc_utils::LogLevel::Error>(player, "获取维度失败"_trf(player));
        }
    }
}

void Selector::onFixesY() { drawAABB(); }

} // namespace land


namespace land {

// 重新选区器
LandReSelector::LandReSelector(Player& player, LandData_sptr const& data)
: Selector(player, data->getLandDimid(), data->is3DLand(), Type::ReSelector),
  mLandData(data) {
    mOldBoxGeoId = DrawHandleManager::getInstance().getOrCreateHandle(player)->draw(
        data->mPos,
        mDimensionId,
        mce::Color::ORANGE()
    );
}

LandReSelector::~LandReSelector() {
    DrawHandleManager::getInstance().getOrCreateHandle(*mPlayer)->remove(mOldBoxGeoId);
}

LandData_sptr LandReSelector::getLandData() const { return mLandData.lock(); }


// 子领地选区器
SubLandSelector::SubLandSelector(Player& player, LandData_sptr const& data)
: Selector(player, data->getLandDimid(), data->is3DLand(), Type::SubLand),
  mParentLandData(data) {
    this->mParentRangeBoxGeoId =
        DrawHandleManager::getInstance().getOrCreateHandle(player)->draw(data->mPos, mDimensionId, mce::Color::RED());
}

SubLandSelector::~SubLandSelector() {
    DrawHandleManager::getInstance().getOrCreateHandle(*mPlayer)->remove(mParentRangeBoxGeoId);
}

LandData_sptr SubLandSelector::getParentLandData() const { return mParentLandData.lock(); }


void SubLandSelector::onABSelected() {
    // Selector::onABSelected(); // Call base class
    SelectorChangeYGui::impl(getPlayer()); // 发送GUI
}

} // namespace land


namespace land {

ll::event::ListenerPtr                 Global_PlayerUseItemOn;
std::unordered_map<UUIDm, std::time_t> Global_Stabilized; // 防抖

SelectorManager::SelectorManager() {
    Global_PlayerUseItemOn = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerInteractBlockEvent>(
        [this](ll::event::PlayerInteractBlockEvent const& ev) {
            auto& player = ev.self();
            if (!this->hasSelector(player)) {
                return;
            }

            // 防抖
            if (auto iter = Global_Stabilized.find(player.getUuid()); iter != Global_Stabilized.end()) {
                if (iter->second >= Date::now().getTime()) {
                    return;
                }
            }
            Global_Stabilized[player.getUuid()] = Date::future(50 / 1000).getTime(); // 50ms

            // 获取选区器
            auto selector = this->get(player);
            if (!selector) {
                throw std::runtime_error("Data sync exception: selector not found"); // 未定义行为
            }

            if (!selector->isSelectorTool(ev.item())) {
                return;
            }
            if (!selector->canSelect()) {
                mc_utils::executeCommand("pland buy", &player);
                return;
            }

            if (selector->canSelectPointA()) {
                selector->selectPointA(ev.blockPos());
                mc_utils::sendText(player, "已选择点A \"{}\""_trf(player, ev.blockPos().toString()));

            } else if (selector->canSelectPointB()) {
                selector->selectPointB(ev.blockPos());
                mc_utils::sendText(player, "已选择点B \"{}\""_trf(player, ev.blockPos().toString()));

                selector->onABSelected(); // 通知已完成 AB 选择
            }
        }
    );

    using ll::chrono_literals::operator""_tick; // 1s = 20_tick
    // repeat task
    ll::coro::keepThis([this]() -> ll::coro::CoroTask<> {
        while (GlobalRepeatCoroTaskRunning) {
            co_await 20_tick;
            if (!GlobalRepeatCoroTaskRunning) co_return;

            for (auto& [uuid, selector] : mSelectors) {
                try {
                    auto player = selector->mPlayer; // 玩家指针
                    if (!player) {
                        mSelectors.erase(uuid); // 玩家断开连接
                        continue;
                    }

                    SetTitlePacket title(SetTitlePacket::TitleType::Title);
                    SetTitlePacket subTitle(SetTitlePacket::TitleType::Subtitle);
                    if (selector->canSelectPointA()) {
                        title.mTitleText = "[ 选区器 ]"_trf(*player);
                        subTitle.mTitleText =
                            "使用 /pland set a 或使用 {} 选择点A"_trf(*player, Config::cfg.selector.tool);

                    } else if (selector->canSelectPointB()) {
                        title.mTitleText = "[ 选区器 ]"_trf(*player);
                        subTitle.mTitleText =
                            "使用 /pland set b 或使用 {} 选择点B"_trf(*player, Config::cfg.selector.tool);

                    } else {
                        title.mTitleText    = "[ 选区完成 ]"_trf(*player);
                        subTitle.mTitleText = "使用 /pland buy 呼出购买菜单"_trf(*player, Config::cfg.selector.tool);
                    }

                    title.sendTo(*player);
                    subTitle.sendTo(*player);
                } catch (...) {
                    mSelectors.erase(uuid);
                    continue;
                }
            }
        }
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

void SelectorManager::cleanup() {
    mSelectors.clear();
    Global_Stabilized.clear();
    ll::event::EventBus::getInstance().removeListener(Global_PlayerUseItemOn);
}


SelectorManager& SelectorManager::getInstance() {
    static SelectorManager instance;
    return instance;
}

bool SelectorManager::hasSelector(Player& player) const {
    return mSelectors.find(player.getUuid()) != mSelectors.end();
}

bool SelectorManager::start(std::unique_ptr<Selector> selector) {
    if (!selector || !selector->mPlayer) {
        return false;
    }

    auto& player = selector->getPlayer();

    if (hasSelector(player)) {
        return false;
    }

    return mSelectors.emplace(selector->mPlayer->getUuid(), std::move(selector)).second;
}

void SelectorManager::cancel(Player& player) {
    auto it = mSelectors.find(player.getUuid());
    if (it != mSelectors.end()) {
        // it->second->onCancel();
        mSelectors.erase(it);
    }

    SetTitlePacket reset(::SetTitlePacket::TitleType::Clear);
    reset.sendTo(player); // clear title
}

Selector* SelectorManager::get(Player& player) const {
    auto it = mSelectors.find(player.getUuid());
    if (it != mSelectors.end()) {
        return it->second.get();
    }
    return nullptr;
}


} // namespace land