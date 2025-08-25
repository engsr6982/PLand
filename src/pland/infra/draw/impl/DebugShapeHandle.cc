#include "DebugShapeHandle.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/phys/AABB.h"
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/infra/draw/impl/debug_shape/DebugShape.h"
#include "pland/land/Land.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <mc/deps/core/utility/AutomaticID.h>
#include <memory>
#include <unordered_map>
#include <utility>


namespace land {


class FixedBox {
    static constexpr auto LineNum = 12;

    GeoId                                                        mId{};
    std::array<std::unique_ptr<debug_shape::DebugLine>, LineNum> mLines{};

    static uint64_t getNextId() {
        static uint64_t id{1};
        return id++;
    }

    AABB fixAABB(LandPos const& min, LandPos const& max) {
        return AABB{
            Vec3{min.x + 0.08, min.y + 0.08, min.z + 0.08},
            Vec3{max.x + 0.98, max.y + 0.98, max.z + 0.98}
        };
    }
    std::vector<std::pair<Vec3, Vec3>> getEdgesWithOffset(LandAABB const& aabb) {
        auto faabb = fixAABB(aabb.min, aabb.max);
        return {
            // bottom
            {                              faabb.min, {faabb.min.x, faabb.min.y, faabb.max.z}},
            {{faabb.max.x, faabb.min.y, faabb.min.z}, {faabb.max.x, faabb.min.y, faabb.max.z}},
            {{faabb.min.x, faabb.max.y, faabb.min.z}, {faabb.min.x, faabb.max.y, faabb.max.z}},
            {{faabb.max.x, faabb.max.y, faabb.min.z},                               faabb.max},
            // top
            {                              faabb.min, {faabb.max.x, faabb.min.y, faabb.min.z}},
            {{faabb.min.x, faabb.max.y, faabb.min.z}, {faabb.max.x, faabb.max.y, faabb.min.z}},
            {{faabb.min.x, faabb.min.y, faabb.max.z}, {faabb.max.x, faabb.min.y, faabb.max.z}},
            {{faabb.min.x, faabb.max.y, faabb.max.z},                               faabb.max},
            // side
            {                              faabb.min, {faabb.min.x, faabb.max.y, faabb.min.z}},
            {{faabb.max.x, faabb.min.y, faabb.min.z}, {faabb.max.x, faabb.max.y, faabb.min.z}},
            {{faabb.min.x, faabb.min.y, faabb.max.z}, {faabb.min.x, faabb.max.y, faabb.max.z}},
            {{faabb.max.x, faabb.min.y, faabb.max.z},                               faabb.max},
        };
    }

public:
    LD_DISALLOW_COPY(FixedBox);
    explicit FixedBox(LandAABB const& aabb) : mId(getNextId()) {
        // 获取偏移后的边，保证 Box 始终能精确显示范围
        auto edges = getEdgesWithOffset(aabb);
        assert(edges.size() == LineNum);
        for (int i = 0; i < LineNum; ++i) {
            mLines[i] = std::make_unique<debug_shape::DebugLine>(edges[i].first, edges[i].second);
        }
    }
    ~FixedBox() { remove(); }

    void setColor(mce::Color const& color) {
        for (auto& line : mLines) {
            line->setColor(color);
        }
    }

    void draw(DimensionType dimId) {
        for (auto& line : mLines) {
            line->draw(dimId);
        }
    }

    void remove() {
        for (auto& line : mLines) {
            line->remove();
        }
    }

    GeoId getId() const { return mId; }
};

struct DebugShapeHandle::Impl {
    std::unordered_map<GeoId, std::unique_ptr<FixedBox>>  mShapes;     // 绘制的形状
    std::unordered_map<LandID, std::unique_ptr<FixedBox>> mLandShapes; // 绘制的领地
};


// interface
DebugShapeHandle::DebugShapeHandle() : impl_(std::make_unique<Impl>()) {}
DebugShapeHandle::~DebugShapeHandle() = default;

GeoId DebugShapeHandle::draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) {
    auto box = std::make_unique<FixedBox>(aabb);
    box->setColor(color);
    box->draw(dimId);

    auto id = box->getId();
    impl_->mShapes.emplace(id, std::move(box));
    return id;
}

void DebugShapeHandle::draw(std::shared_ptr<Land> const& land, mce::Color const& color) {
    auto box = std::make_unique<FixedBox>(land->getAABB());
    box->setColor(color);
    box->draw(land->getDimensionId());

    auto id = box->getId();
    impl_->mLandShapes.emplace(id, std::move(box));
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


} // namespace land