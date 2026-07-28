#include "ABSelector.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/drawer/DrawHandleManager.h"
#include "pland/gui/NewLandGUI.h"
#include "pland/land/Config.h"
#include "pland/selector/AbstractSelector.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/McUtils.h"

#include "ll/api/service/Bedrock.h"

#include "mc/deps/core/math/Color.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/platform/UUID.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"

namespace land {

struct ABSelector::Impl {
    mce::UUID const         mPlayerUuid{};
    LandDimid const         mDimid{};
    bool const              mIs3D{false};
    std::optional<BlockPos> mPointA;
    std::optional<BlockPos> mPointB;
    drawer::GeoId           mRenderId{};
    SetTitlePacket          mTitlePacket{SetTitlePacket::TitleType::Title};
    SetTitlePacket          mSubTitlePacket{SetTitlePacket::TitleType::Subtitle};

    ~Impl() { clearRender(); }

    [[nodiscard]] Player* tryGetPlayer() const {
        return ll::service::getLevel().and_then([this](Level& level) { return level.getPlayer(mPlayerUuid); });
    }

    void initTitlePackets(std::string const& locale) {
        mTitlePacket.mTitleText    = "[ {}选区 ]"_trl(locale, mIs3D ? "3D" : "2D");
        mSubTitlePacket.mTitleText = "输入 /pland set a 或使用 '{}' 选择点 A"_trl(locale, Config::cfg.selector.alias);
    }

    void updateTitlePackets(std::string title, std::string subtitle) {
        mTitlePacket.mTitleText    = std::move(title);
        mSubTitlePacket.mTitleText = std::move(subtitle);
    }

    void clearRender(Player* player = nullptr) {
        if (!mRenderId) return;

        auto* targetPlayer = player ? player : tryGetPlayer();
        if (targetPlayer) {
            PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(*targetPlayer)->remove(mRenderId);
        }
        mRenderId = {}; // 清空存储的渲染ID，防止重复释放
    }
};

// ------------------- Life Cycle -------------------

ABSelector::ABSelector(Player& player, LandDimid dimid, bool is3D)
: AbstractSelector(),
  impl(std::make_unique<Impl>(player.getUuid(), dimid, is3D)) {
    impl->initTitlePackets(player.getLocaleCode());
}

ABSelector::~ABSelector() = default;

// ------------------- Getters -------------------

LandDimid               ABSelector::getDimensionId() const { return impl->mDimid; }
std::optional<BlockPos> ABSelector::getPointA() const { return impl->mPointA; }
std::optional<BlockPos> ABSelector::getPointB() const { return impl->mPointB; }
bool                    ABSelector::isPointASet() const { return impl->mPointA.has_value(); }
bool                    ABSelector::isPointBSet() const { return impl->mPointB.has_value(); }
bool                    ABSelector::isPointABSet() const { return isPointASet() && isPointBSet(); }
bool                    ABSelector::is3D() const { return impl->mIs3D; }
Player*                 ABSelector::getPlayer() const { return impl->tryGetPlayer(); }

bool ABSelector::isSpecifiedSelectTool(std::string const& typeName) const {
    return ConfigProvider::getSelectionConfig().item == typeName;
}

std::optional<LandAABB> ABSelector::newLandAABB() const {
    if (!isPointABSet()) return std::nullopt;

    auto aabb = LandAABB::make(LandPos::make(*impl->mPointA), LandPos::make(*impl->mPointB));
    aabb.fix();
    return aabb;
}

// ------------------- Mutators & Core Logic -------------------

void ABSelector::setPointA(BlockPos const& point) {
    bool const isUpdate = isPointASet();
    impl->mPointA       = point;

    isUpdate ? onPointAUpdated() : onPointASet();

    if (isPointABSet()) onPointABSet();
}

void ABSelector::setPointB(BlockPos const& point) {
    bool const isUpdate = isPointBSet();
    impl->mPointB       = point;

    isUpdate ? onPointBUpdated() : onPointBSet();

    if (isPointABSet()) onPointABSet();
}

void ABSelector::setYRange(int start, int end) {
    if (!isPointABSet()) return;

    impl->mPointA->y = start;
    impl->mPointB->y = end;

    if (auto* player = getPlayer()) {
        feedback_utils::sendText(*player, "已设置选区高度范围: {} ~ {}"_trl(player->getLocaleCode(), start, end));
    }
}

void ABSelector::checkAndSwapY() {
    if (isPointABSet() && impl->mPointA->y > impl->mPointB->y) {
        std::swap(impl->mPointA->y, impl->mPointB->y);
    }
}

void ABSelector::selectNext(Player& player, BlockPos const& p) {
    if (isPointABSet()) {
        mc_utils::executeCommand("pland buy", player); // TODO: 优化
    } else if (!isPointASet()) {
        setPointA(p);
    } else {
        setPointB(p);
    }
}

// ------------------- Event Callbacks -------------------

void ABSelector::onPointASet() {
    if (auto* player = getPlayer()) {
        auto const& locale = player->getLocaleCode();
        feedback_utils::sendText(*player, "已选择点 A: {}"_trl(locale, *impl->mPointA));

        impl->mSubTitlePacket.mTitleText =
            "输入 /pland set b 或使用 '{}' 选择点 B"_trl(locale, Config::cfg.selector.alias);
    }
}

void ABSelector::onPointBSet() {
    if (auto* player = getPlayer()) {
        feedback_utils::sendText(*player, "已选择点 B: {}"_trl(player->getLocaleCode(), *impl->mPointB));
    }
}

void ABSelector::onPointAUpdated() {
    if (auto* player = getPlayer()) {
        feedback_utils::sendText(*player, "已更新点 A: {}"_trl(player->getLocaleCode(), *impl->mPointA));
    }
}

void ABSelector::onPointBUpdated() {
    if (auto* player = getPlayer()) {
        feedback_utils::sendText(*player, "已更新点 B: {}"_trl(player->getLocaleCode(), *impl->mPointB));
    }
}

void ABSelector::onPointABSet() {
    auto* player = getPlayer();
    if (!player) return;

    auto const& locale = player->getLocaleCode();
    impl->updateTitlePackets(
        "[ 选区完成 ]"_trl(locale),
        "输入 /pland buy 呼出购买菜单"_trl(locale, Config::cfg.selector.alias)
    );

    if (!is3D()) {
        auto dimension = player->getLevel().getDimension(getDimensionId()).lock();
        if (!dimension) {
            feedback_utils::sendErrorText(*player, "获取维度失败"_trl(locale));
            return;
        }

        auto const& range = dimension->mHeightRange;
        impl->mPointA->y  = range->mMin;
        impl->mPointB->y  = range->mMax;

        onPointConfirmed();
        return;
    }

    checkAndSwapY();
    gui::NewLandGUI::sendConfirmPrecinctsYRange(*player);
}

void ABSelector::onPointConfirmed() {
    auto* player = getPlayer();
    auto  aabb   = newLandAABB();
    if (!player || !aabb) return;

    impl->clearRender(player);

    auto handle = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(*player);
    auto color  = mce::Color::fromHexString(ConfigProvider::getDrawConfig().color.onSelectorConfirm);

    impl->mRenderId = handle->draw(*aabb, impl->mDimid, color);
}

void ABSelector::tick() {
    if (auto* player = getPlayer()) {
        impl->mTitlePacket.sendTo(*player);
        impl->mSubTitlePacket.sendTo(*player);
    }
}

} // namespace land