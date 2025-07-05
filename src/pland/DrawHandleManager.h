#pragma once
#include "Global.h"
#include "ll/api/base/StdInt.h"
#include "mc/world/actor/player/Player.h"
#include "pland/LandData.h"
#include "pland/math/LandAABB.h"
#include <memory>
#include <unordered_map>


namespace land {


/**
 * @brief 绘制管理器
 * 对 bsci::GeometryGroup 的 wrapper
 */
class DrawHandle {
public:
    // Disallow copy and move
    DrawHandle(const DrawHandle&)            = delete;
    DrawHandle& operator=(const DrawHandle&) = delete;
    DrawHandle(DrawHandle&&)                 = delete;
    DrawHandle& operator=(DrawHandle&&)      = delete;
    explicit DrawHandle()                    = default;
    virtual ~DrawHandle()                    = default;

    struct DrawId {
        LDAPI virtual ~DrawId() = default;

        LDNDAPI virtual uint64 getValue() const                      = 0; // bsci::GeometryGroup::GeoId::value
        LDNDAPI virtual bool   operator==(DrawId const& other) const = 0;
        LDNDAPI virtual        operator bool() const                 = 0;

        template <typename T>
        [[nodiscard]] T* cast() {
            return static_cast<T*>(this);
        }
    };
    using UniqueDrawId = std::unique_ptr<DrawId>;

public:
    /**
     * @brief 绘制一个临时的区域
     */
    LDNDAPI virtual UniqueDrawId draw(LandAABB const& pos, DimensionType dim, const mce::Color& color) = 0;

    /**
     * @brief 绘制领地范围
     */
    LDAPI virtual void draw(LandData_sptr const& land, const mce::Color& color) = 0;

    /**
     * @brief 移除一个领地的绘制
     */
    LDAPI virtual void remove(LandID landID) = 0;

    /**
     * @brief 移除一个临时的区域
     */
    LDAPI virtual void remove(UniqueDrawId const& drawId) = 0;

    /**
     * @brief 移除所有临时的区域
     */
    LDAPI virtual void removeLands() = 0;

    /**
     * @brief 重置所有绘制
     */
    LDAPI virtual void reinit() = 0;
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