#include "DebugShapeHandle.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/phys/AABB.h"
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/Land.h"
#include <cassert>
#include <cstdint>
#include <mc/_HeaderOutputPredefine.h>
#include <mc/deps/core/math/Color.h>
#include <mc/world/phys/AABB.h>
#include <memory>
#include <unordered_map>
#include <utility>

#include "windows.h"

// clang-format off
namespace debug_shape {
class IDrawer {
public:
    virtual ~IDrawer() = default;

    virtual void draw() const                        = 0; // sendToClients (all players)
    virtual void draw(Player& player) const          = 0; // sendToPlayer (only specific player)
    virtual void draw(DimensionType dimension) const = 0; // sendToDimension (only specific dimension)

    virtual void remove() const                        = 0;
    virtual void remove(Player& player) const          = 0;
    virtual void remove(DimensionType dimension) const = 0;

    virtual void update() const                        = 0;
    virtual void update(Player& player) const          = 0;
    virtual void update(DimensionType dimension) const = 0;
};
} // namespace debug_shape
namespace debug_shape::extension {
class IBoundsBox : public IDrawer {
public:
    // DSNDAPI static std::unique_ptr<IBoundsBox>
    // create(AABB const& bounds, mce::Color const& color = mce::Color::WHITE());

    virtual void setBounds(AABB const& bounds) = 0;

    virtual std::optional<Vec3> getRotation() const = 0; // 旋转

    virtual void setRotation(std::optional<Vec3> rot) = 0;

    virtual std::optional<mce::Color> getColor() const = 0; // 颜色

    virtual void setColor(std::optional<mce::Color> c) = 0;

    virtual bool hasDuration() const = 0; // 是否有持续时间

    virtual std::optional<float> getTotalTimeLeft() const = 0; // 剩余时间(s)

    virtual void setTotalTimeLeft(std::optional<float> t) = 0;

    // v1.21.120
    virtual std::optional<DimensionType> getDimensionId() const = 0;

    virtual void setDimensionId(std::optional<DimensionType> d) = 0;
};
} // namespace debug_shape::extension
// clang-format on


namespace land::drawer::detail {

#ifndef _MSC_VER
#error "This code only supports MSVC"
#endif

inline constexpr auto DebugShapeModuleName = L"DebugShape.dll";
inline constexpr auto DebugShapeIBoundsBoxFactroySymbol =
    "?create@IBoundsBox@extension@debug_shape@@SA?AV?$unique_ptr@VIBoundsBox@extension@debug_shape@@U?$default_delete@"
    "VIBoundsBox@extension@debug_shape@@@std@@@std@@AEBVAABB@@AEBVColor@mce@@@Z";

// MSVC placement new abi
using DebugShapeIBoundsBoxFactroy =
    std::unique_ptr<debug_shape::extension::IBoundsBox> (*)(AABB const&, mce::Color const&);

bool isDebugShapeLoaded() { return GetModuleHandle(DebugShapeModuleName) != nullptr; }

using UniqueBoundsBox = std::unique_ptr<debug_shape::extension::IBoundsBox>;

UniqueBoundsBox newBoundsBox(AABB const& aabb, mce::Color const& color = mce::Color::WHITE()) {
    if (!isDebugShapeLoaded()) {
        throw std::runtime_error("DebugShape.dll not loaded");
    }
    auto raw = GetProcAddress(GetModuleHandle(DebugShapeModuleName), DebugShapeIBoundsBoxFactroySymbol);
    if (!raw) {
        throw std::runtime_error("Failed to get address of DebugShapeBoundsBoxCtor");
    }
    auto factroy = reinterpret_cast<DebugShapeIBoundsBoxFactroy>(raw);
    return factroy(aabb, color);
}

inline AABB toMinecraftAABB(LandPos const& min, LandPos const& max) {
    return AABB{
        Vec3{min.x + 0.08, min.y + 0.08, min.z + 0.08},
        Vec3{max.x + 0.98, max.y + 0.98, max.z + 0.98}
    };
}
inline AABB toMinecraftAABB(LandAABB const& aabb) { return toMinecraftAABB(aabb.min, aabb.max); }

inline GeoId allocatedID() {
    static uint64_t next{1};
    return GeoId{next++};
}


struct DebugShapeHandle::Impl {
    std::unordered_map<GeoId, UniqueBoundsBox>  mShapes;     // 绘制的形状
    std::unordered_map<LandID, UniqueBoundsBox> mLandShapes; // 绘制的领地
};


// interface
DebugShapeHandle::DebugShapeHandle() : impl_(std::make_unique<Impl>()) {}
DebugShapeHandle::~DebugShapeHandle() = default;

GeoId DebugShapeHandle::draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) {
    auto box = newBoundsBox(toMinecraftAABB(aabb), color);
    box->setColor(color);
    box->setDimensionId(dimId);
    getTargetPlayer().and_then([&](Player& player) { box->draw(player); });

    auto id = allocatedID();
    impl_->mShapes.emplace(id, std::move(box));
    return id;
}

void DebugShapeHandle::draw(std::shared_ptr<Land> const& land, mce::Color const& color) {
    if (impl_->mLandShapes.contains(land->getId())) {
        return; // 已经绘制过
    }
    auto box = newBoundsBox(toMinecraftAABB(land->getAABB()), color);
    box->setColor(color);
    box->setDimensionId(land->getDimensionId());
    getTargetPlayer().and_then([&](Player& player) { box->draw(player); });
    impl_->mLandShapes.emplace(land->getId(), std::move(box));
}

void DebugShapeHandle::remove(GeoId id) {
    auto iter = impl_->mShapes.find(id);
    if (iter != impl_->mShapes.end()) {
        impl_->mShapes.erase(iter);
    }
}

void DebugShapeHandle::remove(LandID landId) {
    auto iter = impl_->mLandShapes.find(landId);
    if (iter != impl_->mLandShapes.end()) {
        impl_->mLandShapes.erase(iter);
    }
}

void DebugShapeHandle::remove(std::shared_ptr<Land> land) { remove(land->getId()); }

void DebugShapeHandle::clear() {
    impl_->mShapes.clear();
    impl_->mLandShapes.clear();
}

void DebugShapeHandle::clearLand() { impl_->mLandShapes.clear(); }
bool DebugShapeHandle::isDebugShapeLoaded() { return detail::isDebugShapeLoaded(); }


} // namespace land::drawer::detail