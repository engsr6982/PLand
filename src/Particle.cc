#include "pland/Particle.h"
#include "mc/deps/core/utility/typeid_t.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/util/MolangVariable.h"
#include "mc/util/MolangVariableMap.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "pland/Config.h"
#include "pland/LandPos.h"


// Fix LNK 2019
// https://github.com/OEOTYAN/BedrockServerClientInterface/blob/main/src/bsci/particle/ParticleSpawner.cpp#L41-L48
MolangVariableMap::MolangVariableMap(MolangVariableMap const& rhs) {
    mMapFromVariableIndexToVariableArrayOffset = rhs.mMapFromVariableIndexToVariableArrayOffset;
    mVariables                                 = {};
    for (auto& ptr : *rhs.mVariables) {
        mVariables->push_back(std::make_unique<MolangVariable>(*ptr));
    }
    mHasPublicVariables = rhs.mHasPublicVariables;
}


namespace land {

Particle::Particle(BlockPos pos1, BlockPos pos2, int dimid, bool draw3D)
: mPos(LandPos::make(pos1, pos2)),
  mDimid(dimid),
  mDraw3D(draw3D),
  mValid(true) {}
Particle::Particle(LandPos& pos, int dimid, bool draw3D)
: mPos(LandPos(pos)),
  mDimid(dimid),
  mDraw3D(draw3D),
  mValid(true) {}


bool Particle::draw(Player& player, bool refreshCache, bool usePlayerYDraw2D, bool dontCache) {
    {
        auto pos = mDraw3D ? mPos.getBorder() : mPos.getRange();
        auto dim = VanillaDimensions::fromSerializedInt(mDimid);

        if (mPackets.empty() || refreshCache || dontCache) {
            mPackets.reserve(pos.size());
            if (mDraw3D) {
                for (auto& p : pos) {
                    auto pkt = SpawnParticleEffectPacket(
                        Vec3{p.x + 0.5, p.y + 0.5, p.z + 0.5},
                        Config::cfg.selector.particle,
                        (char)dim,
                        std::nullopt
                    );

                    dontCache ? pkt.sendTo(player) : mPackets.push_back(pkt);
                }
            } else {
                // 2D
                float y  = usePlayerYDraw2D ? (float)player.getFeetBlockPos().y : (float)pos[0].y;
                y       += 0.5; // center
                for (auto& p : pos) {
                    auto pkt = SpawnParticleEffectPacket(
                        Vec3{p.x + 0.5, y, p.z + 0.5},
                        Config::cfg.selector.particle,
                        (char)dim,
                        std::nullopt
                    );

                    dontCache ? pkt.sendTo(player) : mPackets.push_back(pkt);
                }
            }
        }
        if (dontCache) {
            return true;
        }
    }

    for (auto& pkt : mPackets) {
        pkt.sendTo(player);
    }
    return true;
}

bool Particle::operator==(const Particle& other) const {
    return mPos == other.mPos && mDimid == other.mDimid && mDraw3D == other.mDraw3D;
}
bool Particle::operator!=(const Particle& other) const { return !(*this == other); }


} // namespace land