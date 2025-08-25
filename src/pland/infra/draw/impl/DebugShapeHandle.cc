#include "DebugShapeHandle.h"
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

public:
    LD_DISALLOW_COPY(FixedBox);
    explicit FixedBox(LandAABB const& aabb) : mId(getNextId()) {
        auto edges = aabb.getEdges();
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