#pragma once
#include "bsci/GeometryGroup.h"
#include "mc/deps/core/utility/typeid_t.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
#include <memory>
#include <optional>
#include <unordered_map>


namespace land {

struct LandSelectorData {
    Player* mPlayer{nullptr};
    LandPos mPos;                   // 选择的位置
    int     mDimid;                 // 维度
    bool    mDraw3D;                // 是否绘制3D
    bool    mSelectedPointA{false}; // 是否已经选择了第一个点
    bool    mSelectedPointB{false}; // 是否已经选择了两个点
    bool    mCanDraw{false};        // 能否绘制
    bool    mCanSelect{true};       // 能否选择

    bool                       mIsDrawedBox{false}; // 是否已经绘制了选区
    bsci::GeometryGroup::GeoId mDrawedBoxGeoId;     // 绘制的GeoId

    // 重新选区
    LandData_wptr              mBindLandData; // 绑定的LandData (此项不为空时，代表玩家正在重新选区<调整大小>)
    bool                       mIsDrawedOldRange{false}; // 是否已经绘制了旧的选区
    bsci::GeometryGroup::GeoId mOldRangeGeoId;           // 旧的GeoId


    LDNDAPI explicit LandSelectorData(Player& player, int dim, bool draw3D);
    LDNDAPI explicit LandSelectorData(Player& player, LandData_sptr const& landData);

    virtual ~LandSelectorData();
};


class LandSelector final {
    LandSelector() = default;

public:
    LandSelector(const LandSelector&)            = delete;
    LandSelector& operator=(const LandSelector&) = delete;
    LandSelector(LandSelector&&)                 = delete;
    LandSelector& operator=(LandSelector&&)      = delete;

    std::unordered_map<UUIDm, std::unique_ptr<LandSelectorData>> mSelectors;

    LDNDAPI static LandSelector& getInstance();

    LDAPI bool init();   // 初始化
    LDAPI bool uninit(); // 卸载

    LDNDAPI LandSelectorData* getSelector(Player& player);

    LDNDAPI bool isSelectTool(ItemStack const& item) const;
    LDNDAPI bool isSelecting(Player& player) const; // 是否正在选区
    LDNDAPI bool isSelected(Player& player) const;  // 是否已经选完
    LDNDAPI bool isSelectedPointA(Player& player) const;
    LDNDAPI bool isSelectedPointB(Player& player) const;

    LDAPI bool isReSelector(Player& player) const;                  // 是否是重新选区
    LDAPI bool tryReSelect(Player& player, LandData_sptr landData); // 重新选区

    LDAPI bool tryStartSelect(Player& player, int dim, bool draw3D); // 开始选区
    LDAPI bool trySelectPointA(Player& player, BlockPos pos);        // 选择第一个点
    LDAPI bool trySelectPointB(Player& player, BlockPos pos);        // 选择第二个点
    LDAPI bool tryCancel(Player& player);                            // 取消选区

    LDAPI bool          completeAndRelease(Player& player);   // 完成选择并释放
    LDAPI LandData_sptr makeLandFromSelector(Player& player); // 从选择器中生成LandData
};


} // namespace land