#pragma once
#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/infra/DrawHandleManager.h"


class Player;
class ItemStack;


namespace land {

struct GeoId;
class LandAABB;

class ISelector {
    WeakRef<EntityContext>  mPlayer{};
    LandDimid               mDimid{-1};
    bool                    m3D = false;
    std::optional<BlockPos> mPointA{std::nullopt};
    std::optional<BlockPos> mPointB{std::nullopt};
    std::unique_ptr<GeoId>  mDrawedRange{nullptr};
    SetTitlePacket          mTitlePacket{SetTitlePacket::TitleType::Title};
    SetTitlePacket          mSubTitlePacket{SetTitlePacket::TitleType::Subtitle};

    void initTitlePacket(Player& player);

public:
    LDAPI explicit ISelector(Player& player);
    LDAPI explicit ISelector(Player& player, LandDimid dimid, bool is3D = false);
    LDAPI virtual ~ISelector();

public:
    LDNDAPI Player*   getPlayer() const;
    LDNDAPI LandDimid getDimensionId() const;
    LDNDAPI std::optional<BlockPos> getPointA() const;
    LDNDAPI std::optional<BlockPos> getPointB() const;

    LDAPI void setPointA(BlockPos const& point);
    LDAPI void setPointB(BlockPos const& point);
    LDAPI void setYRange(int start, int end);

    LDAPI void checkAndSwapY();

    LDNDAPI bool isPointASet() const;
    LDNDAPI bool isPointBSet() const;
    LDNDAPI bool isPointABSet() const;
    LDNDAPI bool is3D() const;

    LDAPI void sendTitle() const;

    LDNDAPI std::optional<LandAABB> newLandAABB() const;

    LDNDAPI std::string dumpDebugInfo() const;

    template <typename T>
    [[nodiscard]] T* as() {
        return dynamic_cast<T*>(this);
    }

public: /* virtual */
    /**
     * @brief 当 A 点被设置时触发
     */
    virtual void onPointASet() /* = 0 */;

    /**
     * @brief 当 B 点被设置时触发
     */
    virtual void onPointBSet() /* = 0 */;

    /**
     * @brief 当 A 点被更新时触发
     * @note 比如：当玩家选择点后，不合适又更新了点
     */
    virtual void onPointAUpdated() /* = 0 */;

    /**
     * @brief 当 B 点被更新时触发
     */
    virtual void onPointBUpdated() /* = 0 */;

    /**
     * @brief 当 A 点和 B 点都设置后触发
     * @warning 此函数可能会触发多次，例如：玩家多次 update a/b 点
     */
    virtual void onPointABSet();

    /**
     * @brief 当 A 点和 B 点都设置后触发
     * @note 当 a 和 b 点都设置后，会向玩家确认坐标，当玩家确认坐标后此函数会被调用
     * @warning 此函数可能会触发多次
     */
    virtual void onPointConfirmed();

    /**
     * @note `SelectorManager` 每 20 tick 调用一次
     * @note 用于向玩家发送标题提示当前状态
     */
    virtual void tick();
};


} // namespace land