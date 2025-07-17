#include "pland/land/LandScheduler.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/PlayerInfo.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/infra/Config.h"
#include "pland/infra/Require.h"
#include "pland/land/LandEvent.h"
#include "pland/land/LandRegistry.h"
#include <cstdio>
#include <stdexcept>


namespace land {


LD_IMPL_REQUIRE(LandScheduler) {
    auto res = land::PLand::getInstance().getLandScheduler();
    if (!res) [[unlikely]] {
        throw std::runtime_error(LD_ERR_RAII_RESOURCE_EMPTY(LandScheduler));
    }
    return res;
}

LandScheduler::LandScheduler() {
    auto* bus = &ll::event::EventBus::getInstance();
    auto* db  = PLand::getInstance().getLandRegistry();

    ll::coro::keepThis([bus, db, this]() -> ll::coro::CoroTask<> {
        while (GlobalRepeatCoroTaskRunning.load()) {
            co_await 5_tick;
            if (!GlobalRepeatCoroTaskRunning.load()) co_return;
            ll::service::getLevel()->forEachPlayer([bus, db, this](Player& player) {
                if (player.isSimulatedPlayer()) return true; // 模拟玩家不处理
                auto& uuid = player.getUuid();

                auto& curPos = player.getPosition(); // 获取玩家当前位置

                int  curDimid  = player.getDimensionId();        // 获取玩家当前维度
                int& lastDimid = LandScheduler::mDimidMap[uuid]; // 获取玩家上一次的维度

                auto& lastLandID = LandScheduler::mLandidMap[uuid]; // 获取玩家上一次所在的领地ID

                auto   land      = db->getLandAt(curPos, curDimid);
                LandID curLandID = land ? land->getId() : -1; // 如果没有领地,设置为-1

                // 处理维度变化
                if (curDimid != lastDimid) {
                    if (lastLandID != (LandID)-1) {
                        auto ev = PlayerLeaveLandEvent(player, lastLandID);
                        bus->publish(ev); // 离开上一个维度的领地
                    }
                    lastDimid = curDimid;
                }

                // 处理领地变化
                if (curLandID != lastLandID) {
                    if (lastLandID != (LandID)-1) {
                        auto ev = PlayerLeaveLandEvent(player, lastLandID);
                        bus->publish(ev); // 离开上一个领地
                    }
                    if (curLandID != (LandID)-1) {
                        auto ev = PlayerEnterLandEvent(player, curLandID);
                        bus->publish(ev); // 进入新领地
                    }
                    lastLandID = curLandID;
                }

                return true;
            });
        }
    }).launch(ll::thread::ServerThreadExecutor::getDefault());

    auto* logger = &land::PLand::getInstance().getSelf().getLogger();

    // tip
    auto* infos = &ll::service::PlayerInfo::getInstance();
    mPlayerEnterLandListener =
        bus->emplaceListener<PlayerEnterLandEvent>([logger, infos, db, this](PlayerEnterLandEvent& ev) {
            logger->debug("Player {} enter land {}", ev.getPlayer().getRealName(), ev.getLandID());
            if (!Config::cfg.land.tip.enterTip) {
                return;
            }

            auto& player = ev.getPlayer();
            if (auto settings = db->getPlayerSettings(player.getUuid().asString());
                settings && !settings->showEnterLandTitle) {
                return; // 如果玩家设置不显示进入领地提示,则不显示
            }

            LandID landid = ev.getLandID();

            SharedLand land = db->getLand(landid);
            if (!land) {
                return;
            }

            SetTitlePacket title(SetTitlePacket::TitleType::Title);
            SetTitlePacket subTitle(SetTitlePacket::TitleType::Subtitle);

            if (land->isOwner(player.getUuid().asString())) {
                title.mTitleText    = land->getName();
                subTitle.mTitleText = "欢迎回来"_trf(player);
            } else {
                auto owner = land->getOwner();
                auto info  = infos->fromUuid(mce::UUID::fromString(owner));
                if (!info.has_value()) {
                    logger->warn("Failed to get the name of player \"{}\", please check the PlayerInfo status.", owner);
                }

                title.mTitleText    = "Welcome to"_trf(player);
                subTitle.mTitleText = land->getName();
            }

            title.sendTo(player);
            subTitle.sendTo(player);
        });

    if (Config::cfg.land.tip.bottomContinuedTip) {
        ll::coro::keepThis([infos, db, this]() -> ll::coro::CoroTask<> {
            while (GlobalRepeatCoroTaskRunning) {
                co_await (Config::cfg.land.tip.bottomTipFrequency * 20_tick);
                if (!GlobalRepeatCoroTaskRunning) co_return;
                auto level = ll::service::getLevel();
                if (!level) {
                    continue;
                }

                SetTitlePacket pkt(SetTitlePacket::TitleType::Actionbar);

                auto& landIds = LandScheduler::mLandidMap;
                for (auto& [curPlayerUUID, landid] : landIds) {
                    if (landid == (LandID)-1) {
                        continue;
                    }

                    auto player = level->getPlayer(curPlayerUUID);
                    if (!player) {
                        continue;
                    }
                    if (auto settings = db->getPlayerSettings(player->getUuid().asString());
                        settings && !settings->showBottomContinuedTip) {
                        continue; // 如果玩家设置不显示底部提示，则跳过
                    }

                    auto land = db->getLand(landid);
                    if (!land) {
                        continue;
                    }

                    auto const owner = UUIDm::fromString(land->getOwner());
                    auto       info  = infos->fromUuid(owner);
                    if (land->isOwner(curPlayerUUID.asString())) {
                        pkt.mTitleText = "[Land] 当前正在领地 {}"_trf(*player, land->getName());
                    } else {
                        pkt.mTitleText =
                            "[Land] 这里是 {} 的领地"_trf(*player, info.has_value() ? info->name : owner.asString());
                    }

                    pkt.sendTo(*player);
                }
            }
        }).launch(ll::thread::ServerThreadExecutor::getDefault());
    }
}

LandScheduler::~LandScheduler() {
    auto& bus = ll::event::EventBus::getInstance();
    bus.removeListener(mPlayerEnterLandListener);
}


} // namespace land