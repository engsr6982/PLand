#include "pland/DrawHandleManager.h"
#include "bsci/GeometryGroup.h"
#include "ll/api/io/Logger.h"
#include "pland/mod/ModEntry.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include "ll/api/base/StdInt.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "pland/Config.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include "pland/math/LandAABB.h"
#include <memory>
#include <stdexcept>


namespace land {


struct DrawIdImpl : public DrawHandle::DrawId {
    bsci::GeometryGroup::GeoId value{};

    explicit DrawIdImpl(uint64 value_) { this->value.value = value_; }
    explicit DrawIdImpl(bsci::GeometryGroup::GeoId value_) : value(value_) {}
    explicit DrawIdImpl(bsci::GeometryGroup::GeoId&& value_) : value(value_) {}

    [[nodiscard]] uint64 getValue() const override { return value.value; }
    bool operator==(DrawId const& other) const override { return other.getValue() == this->value.value; }
    operator bool() const override { return value.value == 0; }
};


class DarwHandleImpl : public DrawHandle {
private:
#ifdef _WIN32
    using CreateDefaultFn = std::unique_ptr<bsci::GeometryGroup> (*)();
    CreateDefaultFn mCreateDefaultFunc{};
#endif
    std::unique_ptr<bsci::GeometryGroup>                               mGeometryGroup;
    std::unordered_map<LandID, std::pair<LandData_wptr, UniqueDrawId>> mLandGeoMap;

public:
    explicit DarwHandleImpl() : DrawHandle() {
#ifdef _WIN32
        HMODULE bsciModule = GetModuleHandleA("BedrockServerClientInterface.dll");
        if (!bsciModule) {
            bsciModule = LoadLibraryA("BedrockServerClientInterface.dll");
        }

        if (bsciModule) {

            mCreateDefaultFunc = (CreateDefaultFn)GetProcAddress(
                bsciModule,
                "?createDefault@GeometryGroup@bsci@@SA?AV?$unique_ptr@VGeometryGroup@bsci@@U?$default_delete@"
                "VGeometryGroup@bsci@@@std@@@std@@XZ"
            );
        }

        if (mCreateDefaultFunc) {
            mGeometryGroup = mCreateDefaultFunc();
        } else {
            auto& logger = mod::ModEntry::getInstance().getSelf().getLogger();
            logger.warn("未能加载 BedrockServerClientInterface.dll，绘图功能将不可用。");
            logger.error("请检查 是否已安装BedrockServerClientInterface且兼容。");
            throw std::runtime_error("Failed to load bsci::GeometryGroup::createDefault from bsci.dll");
        }
#else
        mGeometryGroup = bsci::GeometryGroup::createDefault();
#endif
    }

    // 辅助函数
    AABB fixAABB(LandPos const& min, LandPos const& max) {
        return AABB{
            Vec3{min.x + 0.08, min.y + 0.08, min.z + 0.08},
            Vec3{max.x + 0.98, max.y + 0.98, max.z + 0.98}
        };
    }
    AABB fixAABB(LandAABB const& aabb) { return fixAABB(aabb.min, aabb.max); }


    UniqueDrawId draw(LandAABB const& pos, DimensionType dim, const mce::Color& color) override {
        auto result = mGeometryGroup->box(dim, fixAABB(pos), color);
        return std::make_unique<DrawIdImpl>(result);
    }

    void draw(LandData_sptr const& land, const mce::Color& color) override {
        auto iter = mLandGeoMap.find(land->getLandID());
        if (iter != mLandGeoMap.end()) {
            return;
        }
        auto geoid = draw(land->getLandPos(), land->getLandDimid(), color);
        mLandGeoMap.emplace(land->getLandID(), std::make_pair(land, std::move(geoid)));
    }

    void remove(LandID landID) override {
        auto iter = mLandGeoMap.find(landID);
        if (iter != mLandGeoMap.end()) {
            mGeometryGroup->remove(iter->second.second->cast<DrawIdImpl>()->value);
            mLandGeoMap.erase(iter);
        }
    }

    void remove(UniqueDrawId const& drawId) override { mGeometryGroup->remove(drawId->cast<DrawIdImpl>()->value); }

    void removeLands() override {
        for (auto& [id, pair] : mLandGeoMap) {
            remove(pair.second);
        }
        mLandGeoMap.clear();
    }

    void reinit() override {
        this->mLandGeoMap.clear();
        this->mGeometryGroup.reset();
#ifdef _WIN32
        if (mCreateDefaultFunc) {
            this->mGeometryGroup = mCreateDefaultFunc();
        } else {
            throw std::runtime_error("bsci::GeometryGroup::createDefault not loaded");
        }
#else
        this->mGeometryGroup = bsci::GeometryGroup::createDefault();
#endif
    }
};


DrawHandleManager::DrawHandleManager() = default;

DrawHandleManager& DrawHandleManager::getInstance() {
    static DrawHandleManager instance;
    return instance;
}

DrawHandle* DrawHandleManager::getOrCreateHandle(Player& player) {
    auto iter = mDrawHandles.find(player.getUuid());
    if (iter == mDrawHandles.end()) {
        auto handle = std::make_unique<DarwHandleImpl>();
        iter        = mDrawHandles.emplace(player.getUuid(), std::move(handle)).first;
    }
    return iter->second.get();
}

void DrawHandleManager::removeHandle(Player& player) { mDrawHandles.erase(player.getUuid()); }

void DrawHandleManager::removeAllHandle() { mDrawHandles.clear(); }


} // namespace land
