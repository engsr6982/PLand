#pragma once
#include "AbstractSelector.h"
#include "pland/Global.h"

#include <memory>
#include <type_traits>

namespace mce {
class UUID;
}

namespace land {

class LandAABB;

class ABSelector : public AbstractSelector {
public:
    LDAPI explicit ABSelector(Player& player, LandDimid dimid, bool is3D);
    LDAPI ~ABSelector() override;

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

    LDNDAPI std::optional<LandAABB> newLandAABB() const;

    /**
     * @brief 当 A 点被设置时触发
     */
    LDAPI virtual void onPointASet() /* = 0 */;

    /**
     * @brief 当 B 点被设置时触发
     */
    LDAPI virtual void onPointBSet() /* = 0 */;

    /**
     * @brief 当 A 点被更新时触发
     * @note 比如：当玩家选择点后，不合适又更新了点
     */
    LDAPI virtual void onPointAUpdated() /* = 0 */;

    /**
     * @brief 当 B 点被更新时触发
     */
    LDAPI virtual void onPointBUpdated() /* = 0 */;

    /**
     * @brief 当 A 点和 B 点都设置后触发
     * @warning 此函数可能会触发多次，例如：玩家多次 update a/b 点
     */
    LDAPI virtual void onPointABSet();

    /**
     * @brief 当 A 点和 B 点都设置后触发
     * @note 当 a 和 b 点都设置后，会向玩家确认坐标，当玩家确认坐标后此函数会被调用
     * @warning 此函数可能会触发多次
     */
    LDAPI virtual void onPointConfirmed();

public: /// AbstractSelector
    LDNDAPI Player* getPlayer() const override;

    LDNDAPI bool isSpecifiedSelectTool(std::string const& typeName) const override;

    LDAPI void selectNext(Player& player, BlockPos const& p) override;

    LDAPI void tick() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

static_assert(std::is_abstract_v<ABSelector> == false);

} // namespace land