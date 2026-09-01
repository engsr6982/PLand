#include "DefaultParticleHandle.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/aabb/LandAABB.h"
#include "pland/land/Land.h"

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/thread/ServerThreadExecutor.h"

#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/world/level/dimension/VanillaDimensions.h"

#include <atomic>
#include <optional>

namespace land::drawer::detail {


class ParticleSpawner {
    GeoId                                  mId;
    std::vector<SpawnParticleEffectPacket> mPackets;

    static GeoId getNextGeoId() {
        static uint64 id{1};
        return GeoId{id++};
    }

    void buildPackets(LandAABB const& aabb, LandDimid dimId, std::string const& particle) {
        auto points = aabb.getBorder();
        mPackets.reserve(points.size());
        for (auto& point : points) {
            mPackets.emplace_back(Vec3{point.x + 0.5, point.y + 0.5, point.z + 0.5}, particle, dimId, std::nullopt);
        }
    }

public:
    LD_DISABLE_COPY(ParticleSpawner);
    ParticleSpawner(ParticleSpawner&&) noexcept            = default;
    ParticleSpawner& operator=(ParticleSpawner&&) noexcept = default;

    explicit ParticleSpawner(
        LandAABB const&    aabb,
        LandDimid          dimId,
        std::string const& particle = "minecraft:villager_happy"
    )
    : mId(getNextGeoId()) {
        buildPackets(aabb, dimId, particle);
    }

    GeoId getId() const { return mId; }

    void tick(Player& player) {
        for (auto& packet : mPackets) {
            packet.sendTo(player);
        }
    }
};

class DefaultParticleHandle::Impl {
    std::unordered_map<GeoId, ParticleSpawner>    mSpawners;
    std::unordered_map<LandID, GeoId>             mDrawedLands;
    std::shared_ptr<std::atomic<bool>>            mQuit;
    std::shared_ptr<ll::coro::InterruptableSleep> mSleep;
    DefaultParticleHandle&                        mOwner;

public:
    explicit Impl(DefaultParticleHandle& owner) : mOwner(owner) {
        mQuit  = std::make_shared<std::atomic<bool>>(false);
        mSleep = std::make_shared<ll::coro::InterruptableSleep>();

        ll::coro::keepThis([quit = mQuit, sleep = mSleep, this]() -> ll::coro::CoroTask<> {
            while (!quit->load()) {
                co_await sleep->sleepFor(ll::chrono::ticks{30});
                if (quit->load()) {
                    break;
                }
                mOwner.getTargetPlayer().and_then([this](Player& player) {
                    for (auto& [id, spawner] : mSpawners) {
                        spawner.tick(player);
                    }
                });
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

    void draw(std::shared_ptr<Land> const& land) {
        if (mDrawedLands.contains(land->getId())) {
            return;
        }
        auto id                     = this->draw(land->getAABB(), land->getDimensionId());
        mDrawedLands[land->getId()] = id;
    }

    void remove(GeoId id) {
        PLand::getInstance().getSelf().getLogger().debug(
            "Remove GeoId {}, exist: {}",
            id.value,
            mSpawners.contains(id)
        );
        mSpawners.erase(id);
    }

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

DefaultParticleHandle::DefaultParticleHandle() : impl(std::make_unique<Impl>(*this)) {}

DefaultParticleHandle::~DefaultParticleHandle() = default;

GeoId DefaultParticleHandle::draw(LandAABB const& aabb, DimensionType dimId, mce::Color const&) {
    return impl->draw(aabb, dimId);
}

void DefaultParticleHandle::draw(std::shared_ptr<Land> const& land, mce::Color const&) { impl->draw(land); }

void DefaultParticleHandle::remove(GeoId id) { impl->remove(id); }

void DefaultParticleHandle::remove(LandID landId) { impl->remove(landId); }

void DefaultParticleHandle::remove(std::shared_ptr<Land> land) { impl->remove(land->getId()); }

void DefaultParticleHandle::clear() { impl->clear(); }

void DefaultParticleHandle::clearLand() { impl->clearLand(); }


} // namespace land::drawer::detail
