#pragma once
#include "Global.h"
#include "mc/world/actor/player/Player.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
#include <bsci/GeometryGroup.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>


namespace land {


class DrawHandle {
private:
    std::unique_ptr<bsci::GeometryGroup>                                             mGeometryGroup;
    std::unordered_map<LandID, std::pair<LandData_wptr, bsci::GeometryGroup::GeoId>> mLandGeoMap;

    friend class DrawHandleManager;

public:
    DrawHandle(const DrawHandle&)            = delete;
    DrawHandle& operator=(const DrawHandle&) = delete;
    DrawHandle(DrawHandle&&)                 = delete;
    DrawHandle& operator=(DrawHandle&&)      = delete;

    explicit DrawHandle();
    virtual ~DrawHandle() = default;

public:
    /**
     * @brief 绘制一个临时的区域
     */
    LDAPI bsci::GeometryGroup::GeoId
          draw(LandPos const& pos, DimensionType dim, const mce::Color& color = mce::Color::WHITE());

    /**
     * @brief 绘制领地范围
     */
    LDAPI void draw(LandData_sptr const& land, const mce::Color& color = mce::Color::WHITE());

    LDAPI void remove(LandID landID);

    LDAPI void remove(bsci::GeometryGroup::GeoId geoid);

    LDAPI void removeLands(); // 移除所有领地的绘制

    LDAPI void reinit(); // 重置所有绘制
};


class DrawHandleManager final {
public:
    DrawHandleManager(DrawHandleManager&&)                 = delete;
    DrawHandleManager(const DrawHandleManager&)            = delete;
    DrawHandleManager& operator=(DrawHandleManager&&)      = delete;
    DrawHandleManager& operator=(const DrawHandleManager&) = delete;

private:
    explicit DrawHandleManager();

    std::unordered_map<UUIDm, std::unique_ptr<DrawHandle>> mDrawHandles;

public:
    LDNDAPI static DrawHandleManager& getInstance();

    LDNDAPI DrawHandle* getOrCreateHandle(Player& player);

    LDAPI void removeHandle(Player& player);

    LDAPI void removeAllHandle();
};


} // namespace land