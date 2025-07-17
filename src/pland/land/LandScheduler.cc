#include "pland/land/LandScheduler.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
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
#include <vector>

namespace land {


LandScheduler::LandScheduler() {
    auto& bus = ll::event::EventBus::getInstance();

    mQuit                   = std::make_shared<std::atomic<bool>>(false);
    mEventSchedulingSleep   = std::make_shared<ll::coro::InterruptableSleep>();
    mLandTipSchedulingSleep = std::make_shared<ll::coro::InterruptableSleep>();

    mPlayerJoinServerListener = bus.emplaceListener<ll::event::PlayerJoinEvent>([this](ll::event::PlayerJoinEvent& ev) {
        auto& player = ev.self();
        if (player.isSimulatedPlayer()) {
            return;
        }
        mPlayers.emplace_back(&player);
    });

    mPlayerDisconnectListener =
        bus.emplaceListener<ll::event::PlayerDisconnectEvent>([this](ll::event::PlayerDisconnectEvent& ev) {
            auto& player = ev.self();
            if (player.isSimulatedPlayer()) {
                return;
            }

            auto ptr = &player;
            mDimensionMap.erase(ptr);
            mLandIdMap.erase(ptr);
            std::erase_if(mPlayers, [&ptr](auto* p) { return p == ptr; });
        });

    mPlayerEnterLandListener = bus.emplaceListener<PlayerEnterLandEvent>([](PlayerEnterLandEvent& ev) {
        if (!Config::cfg.land.tip.enterTip) {
            return;
        }

        auto& player   = ev.getPlayer();
        auto  registry = PLand::getInstance().getLandRegistry();

        if (auto settings = registry->getPlayerSettings(player.getUuid().asString());
            settings && !settings->showEnterLandTitle) {
            return; // 如果玩家设置不显示进入领地提示,则不显示
        }

        auto land = registry->getLand(ev.getLandID());
        if (!land) {
            return;
        }

        SetTitlePacket title(SetTitlePacket::TitleType::Title);
        SetTitlePacket subTitle(SetTitlePacket::TitleType::Subtitle);

        if (land->isOwner(player.getUuid().asString())) {
            title.mTitleText    = land->getName();
            subTitle.mTitleText = "欢迎回来"_trf(player);
        } else {
            title.mTitleText    = "Welcome to"_trf(player);
            subTitle.mTitleText = land->getName();
        }

        title.sendTo(player);
        subTitle.sendTo(player);
    });

    ll::coro::keepThis([quit = mQuit, sleep = mEventSchedulingSleep, this]() -> ll::coro::CoroTask<> {
        while (!quit->load()) {
            co_await sleep->sleepFor(5_tick);
            if (quit->load()) {
                break;
            }

            if (mPlayers.empty()) {
                continue;
            }

            try {
                tickEvent();
            } catch (std::exception& e) {
                PLand::getInstance().getSelf().getLogger().error(
                    "An exception occurred while scheduling land events: {}",
                    e.what()
                );
            } catch (...) {
                PLand::getInstance().getSelf().getLogger().error(
                    "An unknown exception occurred while scheduling land events."
                );
            }
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());

    if (Config::cfg.land.tip.bottomContinuedTip) {
        ll::coro::keepThis([quit = mQuit, sleep = mLandTipSchedulingSleep, this]() -> ll::coro::CoroTask<> {
            while (!quit->load()) {
                co_await sleep->sleepFor(Config::cfg.land.tip.bottomTipFrequency * 20_tick);
                if (quit->load()) {
                    break;
                }

                if (mLandIdMap.empty()) {
                    continue;
                }

                try {
                    tickLandTip();
                } catch (std::exception& e) {
                    PLand::getInstance().getSelf().getLogger().error(
                        "An exception occurred while scheduling land tip: {}",
                        e.what()
                    );
                } catch (...) {
                    PLand::getInstance().getSelf().getLogger().error(
                        "An unknown exception occurred while scheduling land tip."
                    );
                }
            }
        }).launch(ll::thread::ServerThreadExecutor::getDefault());
    }
}

LandScheduler::~LandScheduler() {
    auto& bus = ll::event::EventBus::getInstance();
    bus.removeListener(mPlayerEnterLandListener);
    bus.removeListener(mPlayerJoinServerListener);
    bus.removeListener(mPlayerDisconnectListener);
    mQuit->store(true);
    mEventSchedulingSleep->interrupt(true);
    mLandTipSchedulingSleep->interrupt(true);
    mPlayers.clear();
    mDimensionMap.clear();
    mLandIdMap.clear();
}

void LandScheduler::tickEvent() {
    auto& bus      = ll::event::EventBus::getInstance();
    auto  registry = PLand::getInstance().getLandRegistry();

    auto iter = mPlayers.begin();
    while (iter != mPlayers.end()) {
        try {
            auto player = *iter;

            auto const& currentPos   = player->getPosition();
            int const   currentDimId = player->getDimensionId();

            int&  lastDimId  = this->mDimensionMap[player];
            auto& lastLandID = this->mLandIdMap[player];

            auto   land          = registry->getLandAt(currentPos, currentDimId);
            LandID currentLandId = land ? land->getId() : -1;

            // 处理维度变化
            if (currentDimId != lastDimId) {
                if (lastLandID != (LandID)-1) {
                    bus.publish(PlayerLeaveLandEvent{*player, lastLandID}); // 离开上一个维度的领地
                }
                lastDimId = currentDimId;
            }

            // 处理领地变化
            if (currentLandId != lastLandID) {
                if (lastLandID != (LandID)-1) {
                    bus.publish(PlayerLeaveLandEvent{*player, lastLandID}); // 离开上一个领地
                }
                if (currentLandId != (LandID)-1) {
                    bus.publish(PlayerEnterLandEvent{*player, currentLandId}); // 进入新领地
                }
                lastLandID = currentLandId;
            }
            ++iter;
        } catch (...) {
            iter = mPlayers.erase(iter);
        }
    }
}

void LandScheduler::tickLandTip() {
    auto& playerInfo = ll::service::PlayerInfo::getInstance();
    auto  registry   = PLand::getInstance().getLandRegistry();

    SetTitlePacket pkt(SetTitlePacket::TitleType::Actionbar);
    for (auto& [player, landId] : mLandIdMap) {
        if (landId == (LandID)-1) {
            continue;
        }

        if (auto settings = registry->getPlayerSettings(player->getUuid().asString());
            settings && !settings->showBottomContinuedTip) {
            continue; // 如果玩家设置不显示底部提示，则跳过
        }

        auto land = registry->getLand(landId);
        if (!land) {
            continue;
        }

        auto& owner = land->getOwner();
        auto  info  = playerInfo.fromUuid(UUIDm::fromString(owner));
        if (land->isOwner(player->getUuid().asString())) {
            pkt.mTitleText = "[Land] 当前正在领地 {}"_trf(*player, land->getName());
        } else {
            pkt.mTitleText = "[Land] 这里是 {} 的领地"_trf(*player, info.has_value() ? info->name : owner);
        }

        pkt.sendTo(*player);
    }
}


} // namespace land