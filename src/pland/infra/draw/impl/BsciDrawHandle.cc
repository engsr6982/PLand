#include "BsciDrawHandle.h"
#include "mc/_HeaderOutputPredefine.h"
#include "mc/world/phys/AABB.h"
#include "pland/aabb/LandAABB.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/infra/draw/impl/BSCIDrawHandle.h"
#include "pland/land/Land.h"
#include <mc/deps/core/utility/AutomaticID.h>

#include "windows.h"

// clang-format off
namespace bsci {
class GeometryGroup {
public:
    using GeoId = ::land::GeoId;

protected:
    // vIndex: - 
    // symbol: ?getNextGeoId@GeometryGroup@bsci@@IEBA?AUGeoId@12@XZ
    GeoId getNextGeoId() const;

public:
    // vIndex: - 
    // symbol: ?createDefault@GeometryGroup@bsci@@SA?AV?$unique_ptr@VGeometryGroup@bsci@@U?$default_delete@VGeometryGroup@bsci@@@std@@@std@@XZ
    static std::unique_ptr<GeometryGroup> createDefault();

    // vIndex: 0
    // symbol: ??1GeometryGroup@bsci@@UEAA@XZ
    virtual ~GeometryGroup() = default;

    // vIndex: 1
    // symbol: 
    virtual GeoId point(
        DimensionType        dim,
        Vec3 const&          pos,
        mce::Color const&    color  = mce::Color::WHITE(),
        std::optional<float> radius = {}
    ) = 0;

    // vIndex: 2
    // symbol: 
    virtual GeoId line(
        DimensionType        dim,
        Vec3 const&          begin,
        Vec3 const&          end,
        mce::Color const&    color     = mce::Color::WHITE(),
        std::optional<float> thickness = {}
    ) = 0;

    // vIndex: 3
    // symbol: 
    virtual bool remove(GeoId) = 0;

    // vIndex: 4
    // symbol: 
    virtual GeoId merge(std::span<GeoId>) = 0;

    // vIndex: 5
    // symbol: 
    virtual bool shift(GeoId, Vec3 const&) = 0;

    // vIndex: 6
    // symbol: ?line@GeometryGroup@bsci@@UEAA?AUGeoId@12@V?$AutomaticID@VDimension@@H@@V?$span@VVec3@@$0?0@std@@AEBVColor@mce@@V?$optional@M@6@@Z
    virtual GeoId line(
        DimensionType        dim,
        std::span<Vec3>      dots,
        mce::Color const&    color     = mce::Color::WHITE(),
        std::optional<float> thickness = {}
    );

    // vIndex: 7
    // symbol: ?box@GeometryGroup@bsci@@UEAA?AUGeoId@12@V?$AutomaticID@VDimension@@H@@AEBVAABB@@AEBVColor@mce@@V?$optional@M@std@@@Z
    virtual GeoId
    box(DimensionType        dim,
        AABB const&          box,
        mce::Color const&    color     = mce::Color::WHITE(),
        std::optional<float> thickness = {});

    // vIndex: 8
    // symbol: ?circle@GeometryGroup@bsci@@UEAA?AUGeoId@12@V?$AutomaticID@VDimension@@H@@AEBVVec3@@1MAEBVColor@mce@@V?$optional@M@std@@@Z
    virtual GeoId circle(
        DimensionType        dim,
        Vec3 const&          center,
        Vec3 const&          normal,
        float                radius,
        mce::Color const&    color     = mce::Color::WHITE(),
        std::optional<float> thickness = {}
    );

    // vIndex: 9
    // symbol: ?cylinder@GeometryGroup@bsci@@UEAA?AUGeoId@12@V?$AutomaticID@VDimension@@H@@AEBVVec3@@1MAEBVColor@mce@@V?$optional@M@std@@@Z
    virtual GeoId cylinder(
        DimensionType        dim,
        Vec3 const&          topCenter,
        Vec3 const&          bottomCenter,
        float                radius,
        mce::Color const&    color     = mce::Color::WHITE(),
        std::optional<float> thickness = {}
    );

    // vIndex: 10
    // symbol: ?sphere@GeometryGroup@bsci@@UEAA?AUGeoId@12@V?$AutomaticID@VDimension@@H@@AEBVVec3@@MAEBVColor@mce@@V?$optional@M@std@@@Z
    virtual GeoId sphere(
        DimensionType        dim,
        Vec3 const&          center,
        float                radius,
        mce::Color const&    color     = mce::Color::WHITE(),
        std::optional<float> thickness = {}
    );
};
} // namespace bsci
// clang-format on


namespace land {


using CreateGeometryGroupFn            = std::unique_ptr<bsci::GeometryGroup> (*)();
static const wchar_t* BSCI_MODULE_NAME = L"BedrockServerClientInterface.dll";

class BsciDrawHandle::Impl {
public:
    static bool isBsciModuleLoadedImpl() { return GetModuleHandle(BSCI_MODULE_NAME) != nullptr; }

    static std::unique_ptr<bsci::GeometryGroup> createDefault() {
        if (!isBsciModuleLoadedImpl()) {
            throw std::runtime_error("BedrockServerClientInterface not loaded.");
        }

        try {
            auto func = reinterpret_cast<CreateGeometryGroupFn>(GetProcAddress(
                GetModuleHandle(BSCI_MODULE_NAME),
                "?createDefault@GeometryGroup@bsci@@SA?AV?$unique_ptr@VGeometryGroup@bsci@@U?$default_delete@"
                "VGeometryGroup@bsci@@@std@@@std@@XZ"
            ));
            return func();
        } catch (std::exception const& e) {
            throw std::runtime_error("Failed to create GeometryGroup: " + std::string(e.what()));
        } catch (...) {
            throw std::runtime_error("Failed to create GeometryGroup.");
        }
    }


    std::unique_ptr<bsci::GeometryGroup> mGeometryGroup;
    std::unordered_map<LandID, GeoId>    mLandGeoMap;

    Impl() { mGeometryGroup = createDefault(); }
    ~Impl() = default;

    void reset() {
        mLandGeoMap.clear();
        mGeometryGroup.reset();
        mGeometryGroup = createDefault();
    }
};


BsciDrawHandle::BsciDrawHandle() : impl(std::make_unique<Impl>()) {}

BsciDrawHandle::~BsciDrawHandle() = default;

GeoId BsciDrawHandle::draw(LandAABB const& aabb, DimensionType dimId, mce::Color const& color) {
    return impl->mGeometryGroup->box(dimId, fixAABB(aabb), color);
}

void BsciDrawHandle::draw(std::shared_ptr<Land> const& land, mce::Color const& color) {
    auto id = draw(land->getAABB(), land->getDimensionId(), color);
    impl->mLandGeoMap.emplace(land->getId(), id);
}

void BsciDrawHandle::remove(GeoId id) {
    if (id) {
        impl->mGeometryGroup->remove(id);
    }
}

void BsciDrawHandle::remove(LandID landId) {
    auto iter = impl->mLandGeoMap.find(landId);
    if (iter != impl->mLandGeoMap.end()) {
        remove(iter->second);
        impl->mLandGeoMap.erase(iter);
    }
}

void BsciDrawHandle::remove(std::shared_ptr<Land> land) { remove(land->getId()); }

void BsciDrawHandle::clear() { impl->reset(); }

void BsciDrawHandle::clearLand() {
    for (auto& [id, geoId] : impl->mLandGeoMap) {
        impl->mGeometryGroup->remove(geoId);
    }
    impl->mLandGeoMap.clear();
}

bool BsciDrawHandle::isBsciModuleLoaded() { return Impl::isBsciModuleLoadedImpl(); }

AABB BsciDrawHandle::fixAABB(LandPos const& min, LandPos const& max) {
    return AABB{
        Vec3{min.x + 0.08, min.y + 0.08, min.z + 0.08},
        Vec3{max.x + 0.98, max.y + 0.98, max.z + 0.98}
    };
}
AABB BsciDrawHandle::fixAABB(LandAABB const& aabb) { return fixAABB(aabb.min, aabb.max); }


} // namespace land
