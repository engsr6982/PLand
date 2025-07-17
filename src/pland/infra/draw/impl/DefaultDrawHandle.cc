#include "DefaultDrawHandle.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/util/MolangVariable.h"
#include "mc/util/MolangVariableMap.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/infra/draw/IDrawHandle.h"
#include "pland/land/Land.h"
#include <atomic>

// Fix LNK2019: "public: __cdecl MolangVariableMap::MolangVariableMap(class MolangVariableMap const &)"
MolangVariableMap::MolangVariableMap(MolangVariableMap const& rhs) {
    mMapFromVariableIndexToVariableArrayOffset = rhs.mMapFromVariableIndexToVariableArrayOffset;
    mVariables                                 = {};
    for (auto& ptr : *rhs.mVariables) {
        mVariables->push_back(std::make_unique<MolangVariable>(*ptr));
    }
    mHasPublicVariables = rhs.mHasPublicVariables;
}

namespace land {


class ParticleSpawner {
    GeoId                                  mId;
    std::vector<SpawnParticleEffectPacket> mPackets;
    int                                    mDimensionId;

    static GeoId getNextGeoId() {
        static uint64 id{0};
        return GeoId{id++};
    }

public:
    LD_DISALLOW_COPY(ParticleSpawner);
    ParticleSpawner(ParticleSpawner&&) noexcept            = default;
    ParticleSpawner& operator=(ParticleSpawner&&) noexcept = default;

    explicit ParticleSpawner(LandAABB const& aabb, LandDimid dimId) : mId(getNextGeoId()), mDimensionId(dimId) {
        static std::optional<MolangVariableMap> molang{std::nullopt};
        if (!molang) {
            molang = MolangVariableMap{}; // TODO: 验证 Molang 是否真的有效
            molang->setMolangVariable("variable.particle_lifetime", 25);
        }

        auto dim    = VanillaDimensions::fromSerializedInt(dimId);
        auto points = aabb.getBorder();
        mPackets.reserve(points.size());
        for (auto& point : points) {
            mPackets.emplace_back(
                Vec3{point.x + 0.5, point.y + 0.5, point.z + 0.5},
                "minecraft:villager_happy",
                dim,
                molang
            );
        }
    }

    GeoId getId() const { return mId; }

    void tick() {
        for (auto& packet : mPackets) {
            packet.sendTo(*packet.mPos, mDimensionId);
        }
    }
};

class DefaultDrawHandle::Impl {
    std::unordered_map<GeoId, ParticleSpawner>    mSpawners;
    std::unordered_map<LandID, GeoId>             mDrawedLands;
    std::shared_ptr<std::atomic<bool>>            mQuit;
    std::shared_ptr<ll::coro::InterruptableSleep> mSleep;

public:
    explicit Impl() {
        mQuit  = std::make_shared<std::atomic<bool>>(false);
        mSleep = std::make_shared<ll::coro::InterruptableSleep>();

        ll::coro::keepThis([quit = mQuit, sleep = mSleep, this]() -> ll::coro::CoroTask<> {
            while (!quit->load()) {
                co_await sleep->sleepFor(30_tick);
                if (quit->load()) {
                    break;
                }

                for (auto& [id, spawner] : mSpawners) {
                    spawner.tick();
                }
            }
            co_return;
        }).launch(ll::thread::ServerThreadExecutor::getDefault());
    }

    ~Impl() {
        mQuit->store(true);
        mSleep->interrupt(true);
    }

    GeoId draw(LandAABB const& aabb, LandDimid dimId) {
        auto spawner = ParticleSpawner(aabb, dimId);
        auto id      = spawner.getId();
        mSpawners.insert({id, std::move(spawner)});
        return id;
    }

    void draw(SharedLand const& land) {
        if (mDrawedLands.contains(land->getId())) {
            return;
        }
        auto id                     = this->draw(land->getAABB(), land->getDimensionId());
        mDrawedLands[land->getId()] = id;
    }

    void remove(GeoId id) { mSpawners.erase(id); }

    void remove(LandID landId) {
        auto iter = mDrawedLands.find(landId);
        if (iter != mDrawedLands.end()) {
            this->remove(iter->second);
            mDrawedLands.erase(iter);
        }
    }

    void clear() {
        mSpawners.clear();
        mDrawedLands.clear();
    }

    void clearLand() {
        auto iter = mDrawedLands.begin();
        while (iter != mDrawedLands.end()) {
            auto& [landId, id] = *iter;
            this->remove(id);
            iter = mDrawedLands.erase(iter);
        }
        mDrawedLands.clear();
    }
};

DefaultDrawHandle::DefaultDrawHandle() : impl(std::make_unique<Impl>()) {}

DefaultDrawHandle::~DefaultDrawHandle() = default;

GeoId DefaultDrawHandle::draw(LandAABB const& aabb, DimensionType dimId, mce::Color const&) {
    return impl->draw(aabb, dimId);
}

void DefaultDrawHandle::draw(std::shared_ptr<Land> const& land, mce::Color const&) { impl->draw(land); }

void DefaultDrawHandle::remove(GeoId id) { impl->remove(id); }

void DefaultDrawHandle::remove(LandID landId) { impl->remove(landId); }

void DefaultDrawHandle::remove(std::shared_ptr<Land> land) { impl->remove(land->getId()); }

void DefaultDrawHandle::clear() { impl->clear(); }

void DefaultDrawHandle::clearLand() { impl->clearLand(); }


} // namespace land