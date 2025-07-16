#include "ISelector.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/gui/NewLandGUI.h"
#include "pland/infra/Config.h"
#include "pland/infra/DrawHandleManager.h"
#include "pland/utils/McUtils.h"


namespace land {

void ISelector::initTitlePacket(Player& player) {
    mTitlePacket.mTitleText    = "[ {}选区 ]"_trf(player, is3D() ? "3D" : "2D");
    mSubTitlePacket.mTitleText = "输入 /pland set a 或使用 '{}' 选择点 A"_trf(player, Config::cfg.selector.alias);
}

ISelector::ISelector(Player& player) : mPlayer(player.getWeakEntity()), mDimid(player.getDimensionId()) {
    initTitlePacket(player);
}

ISelector::ISelector(Player& player, LandDimid dimid, bool is3D)
: mPlayer(player.getWeakEntity()),
  mDimid(dimid),
  m3D(is3D) {
    initTitlePacket(player);
}

ISelector::~ISelector() {
    auto player = getPlayer();
    if (!player) {
        return;
    }
    if (mDrawedRange) {
        DrawHandleManager::getInstance().getOrCreateHandle(*player)->remove(mDrawedRange);
    }
}

Player* ISelector::getPlayer() const { return mPlayer.tryUnwrap<Player>().as_ptr(); }

LandDimid ISelector::getDimensionId() const { return mDimid; }

std::optional<BlockPos> ISelector::getPointA() const { return mPointA; }

std::optional<BlockPos> ISelector::getPointB() const { return mPointB; }


void ISelector::setPointA(BlockPos const& point) {
    if (mPointA) {
        mPointA = point;
        onPointAUpdated(); // a 点更新
    } else {
        mPointA = point;
        onPointASet(); // a 点设置
    }
    if (mPointA && mPointB) {
        onPointABSet(); // a b 点都设置
    }
}

void ISelector::setPointB(BlockPos const& point) {
    if (mPointB) {
        mPointB = point;
        onPointBUpdated();
    } else {
        mPointB = point;
        onPointBSet();
    }
    if (mPointA && mPointB) {
        onPointABSet();
    }
}

void ISelector::setYRange(int start, int end) {
    if (!isPointABSet()) {
        return;
    }
    mPointA->y = start;
    mPointB->y = end;
    if (auto player = getPlayer()) {
        mc_utils::sendText(player, "已设置选区高度范围: {} ~ {}"_trf(*player, mPointA->y, mPointB->y));
    }
}

void ISelector::checkAndSwapY() {
    if (mPointA && mPointB) {
        if (mPointA->y > mPointB->y) std::swap(mPointA->y, mPointB->y);
    }
}

bool ISelector::isPointASet() const { return mPointA.has_value(); }
bool ISelector::isPointBSet() const { return mPointB.has_value(); }
bool ISelector::isPointABSet() const { return mPointA.has_value() && mPointB.has_value(); }
bool ISelector::is3D() const { return m3D; }

void ISelector::sendTitle() const {
    if (auto player = getPlayer()) {
        mTitlePacket.sendTo(*player);
        mSubTitlePacket.sendTo(*player);
    }
}

std::optional<LandAABB> ISelector::newLandAABB() const {
    if (mPointA && mPointB) {
        auto aabb = LandAABB::make(*mPointA, *mPointB);
        aabb.fix();
        return aabb;
    }
    return std::nullopt;
}

std::string ISelector::dumpDebugInfo() const {
    return "DimensionId: {}, PointA: {}, PointB: {}, is3D: {}"_tr(
        mDimid,
        mPointA.has_value() ? mPointA->toString() : "nullopt",
        mPointB.has_value() ? mPointB->toString() : "nullopt",
        is3D()
    );
}


// virtual
void ISelector::onPointASet() {
    if (auto player = getPlayer()) {
        mc_utils::sendText(player, "已选择点 A: {}"_trf(*player, *mPointA));

        // 更新副标题
        mSubTitlePacket.mTitleText = "输入 /pland set b 或使用 '{}' 选择点 B"_trf(*player, Config::cfg.selector.alias);
    }
}

void ISelector::onPointBSet() {
    if (auto player = getPlayer()) {
        mc_utils::sendText(player, "已选择点 B: {}"_trf(*player, *mPointB));
    }
}

void ISelector::onPointAUpdated() {
    if (auto player = getPlayer()) {
        mc_utils::sendText(player, "已更新点 A: {}"_trf(*player, *mPointA));
    }
}

void ISelector::onPointBUpdated() {
    if (auto player = getPlayer()) {
        mc_utils::sendText(player, "已更新点 B: {}"_trf(*player, *mPointB));
    }
}

void ISelector::onPointABSet() {
    auto player = getPlayer();
    if (!player) {
        return;
    }

    mTitlePacket.mTitleText    = "[ 选区完成 ]"_trf(*player);
    mSubTitlePacket.mTitleText = "输入 /pland buy 呼出购买菜单"_trf(*player, Config::cfg.selector.alias);


    if (is3D()) {
        auto dimension = player->getLevel().getDimension(getDimensionId()).lock();
        if (dimension) {
            auto& range = dimension->mHeightRange;

            this->mPointA->y = range->mMin;
            this->mPointB->y = range->mMax;

            onPointConfirmed();
        } else {
            mc_utils::sendText<mc_utils::LogLevel::Error>(player, "获取维度失败"_trf(*player));
        }
        return;
    }

    checkAndSwapY();
    NewLandGUI::sendConfirmPrecinctsYRange(*player);
}

void ISelector::onPointConfirmed() {
    auto player = getPlayer();
    if (!player) {
        return;
    }

    auto handle = DrawHandleManager::getInstance().getOrCreateHandle(*player);

    if (mDrawedRange) {
        handle->remove(mDrawedRange);
    }
    mDrawedRange = handle->draw(*newLandAABB(), mDimid, mce::Color::GREEN());
}

void ISelector::tick() { sendTitle(); }

} // namespace land