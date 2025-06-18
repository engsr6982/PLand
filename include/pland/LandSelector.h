#pragma once
#include "mc/deps/core/utility/typeid_t.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "pland/DrawHandleManager.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/math/LandAABB.h"
#include <memory>
#include <optional>
#include <unordered_map>


namespace land {


class Selector {
public:
    enum class Type : int {
        Default    = 0, // 默认
        ReSelector = 1, // 重新选区
        SubLand    = 2, // 子领地
    };

private:
    friend class SelectorManager;
    friend class SelectorChangeYGui;

protected:
    Type const mType;
    Player*    mPlayer{nullptr}; // 玩家
    int        mDimensionId;     // 维度
    bool       mIs3D;            // 3D 圈地

    std::optional<BlockPos> mPointA; // 第一个点
    std::optional<BlockPos> mPointB; // 第二个点

    DrawHandle::UniqueDrawId mDrawedAABBGeoId; // 绘制的GeoId

public:
    Selector(Selector&)             = delete;
    Selector& operator=(Selector&)  = delete;
    Selector(Selector&&)            = delete;
    Selector& operator=(Selector&&) = delete;

    LDAPI virtual ~Selector();

    LDNDAPI explicit Selector(Player& player, int dimid, bool is3D, Type type = Type::Default);

    LDNDAPI static std::unique_ptr<Selector> createDefault(Player& player, int dimid, bool is3D);

public:
    /* 获取玩家 */
    LDNDAPI Player& getPlayer() const;

    /* 维度 */
    LDNDAPI int getDimensionId() const;

    /* 是否是3D选区 */
    LDNDAPI bool is3D() const;

    /* 获取A点 */
    LDNDAPI std::optional<BlockPos> getPointA() const;

    /* 获取B点 */
    LDNDAPI std::optional<BlockPos> getPointB() const;

    /* 获取选区 */
    LDNDAPI std::optional<LandAABB> getAABB() const;

    /* 修正AABB大小端 */
    LDAPI void fixAABBMinMax();

    /* 从选区数据创建领地 */
    LDNDAPI LandData_sptr newLandData() const;

    /* 获取选择器类别 */
    LDNDAPI Type getType() const;

    /* 转换选择器 */
    template <typename T>
    [[nodiscard]] T* As() {
        return dynamic_cast<T*>(this);
    }


public:
    /* 是否为选区工具 */
    LDNDAPI virtual bool isSelectorTool(ItemStack const& item) const;

    /* 能否进行选区 (AB点) */
    LDNDAPI virtual bool canSelect() const;

    /* 能否选择A点 */
    LDNDAPI virtual bool canSelectPointA() const;

    /* 能否选择B点 */
    LDNDAPI virtual bool canSelectPointB() const;

    /* 选择A点 */
    LDAPI virtual void selectPointA(BlockPos const& pos);

    /* 选择B点 */
    LDAPI virtual void selectPointB(BlockPos const& pos);

    /* 绘制选区 */
    LDAPI virtual void drawAABB();

    /* 事件 */
    LDAPI virtual void onABSelected(); // AB点选择完成
    LDAPI virtual void onFixesY();     // 修正Y轴
};


class LandReSelector final : public Selector {
    LandData_wptr mLandData;

    DrawHandle::UniqueDrawId mOldBoxGeoId;

public:
    LDNDAPI explicit LandReSelector(Player& player, LandData_sptr const& data);

    LDAPI ~LandReSelector() override;

    LDNDAPI LandData_sptr getLandData() const;
};

class SubLandSelector final : public Selector {
    LandData_wptr            mParentLandData;
    DrawHandle::UniqueDrawId mParentRangeBoxGeoId;

public:
    LDNDAPI explicit SubLandSelector(Player& player, LandData_sptr const& data);

    LDAPI ~SubLandSelector() override;

    LDNDAPI LandData_sptr getParentLandData() const;

    LDAPI void onABSelected() override;
};


/**
 * @brief 选区管理器
 * 管理玩家选区的生命周期，以及选区标题提示
 */
class SelectorManager final {
    std::unordered_map<UUIDm, std::unique_ptr<Selector>> mSelectors;

    explicit SelectorManager();

public:
    SelectorManager(const SelectorManager&)            = delete;
    SelectorManager& operator=(const SelectorManager&) = delete;
    SelectorManager(SelectorManager&&)                 = delete;
    SelectorManager& operator=(SelectorManager&&)      = delete;

    LDNDAPI static SelectorManager& getInstance();

    LDAPI void cleanup(); // 清理资源(插件卸载时调用)

    LDNDAPI bool hasSelector(Player& player) const;

    LDNDAPI bool start(std::unique_ptr<Selector> selector);

    LDAPI void cancel(Player& player);

    LDAPI Selector* get(Player& player) const;
};


} // namespace land