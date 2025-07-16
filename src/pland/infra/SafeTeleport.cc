#include "SafeTeleport.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/mod/PLand.h"
#include "pland/utils/McUtils.h"
#include <cstdint>
#include <ll/api/coro/CoroTask.h>
#include <ll/api/thread/ThreadPoolExecutor.h>
#include <mc/deps/core/math/Vec3.h>
#include <mc/deps/game_refs/WeakRef.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/block/Block.h>
#include <mc/world/level/chunk/ChunkSource.h>
#include <mc/world/level/chunk/ChunkState.h>
#include <mc/world/level/chunk/LevelChunk.h>
#include <mc/world/level/dimension/Dimension.h>

namespace land {


bool SafeTeleport::Task::operator==(const Task& other) const { return mId == other.mId; }

inline SafeTeleport::TaskId NextTaskId = 0;
SafeTeleport::Task::Task(Player& player, DimensionPos targetPos)
: mId(NextTaskId++),
  mWeakPlayer(player.getWeakEntity()),
  mTargetDimension(player.getLevel().getDimension(targetPos.second)),
  mTargetChunkPos(ChunkPos(targetPos.first)),
  mSourcePos({player.getPosition(), player.getDimensionId()}),
  mTargetPos(targetPos) {
    mTargetPos.first.x += 0.5; // 方块中心
    mTargetPos.first.z += 0.5;
    mTargetPos.first.y  = 3389;
}


bool SafeTeleport::Task::isPending() const { return mState == TaskState::Pending; }
bool SafeTeleport::Task::isWaitingChunkLoad() const { return mState == TaskState::WaitingChunkLoad; }
bool SafeTeleport::Task::isChunkLoadTimeout() const { return mState == TaskState::ChunkLoadTimeout; }
bool SafeTeleport::Task::isChunkLoaded() const { return mState == TaskState::ChunkLoaded; }
bool SafeTeleport::Task::isFindingSafePos() const { return mState == TaskState::FindingSafePos; }
bool SafeTeleport::Task::isFoundSafePos() const { return mState == TaskState::FoundSafePos; }
bool SafeTeleport::Task::isNoSafePos() const { return mState == TaskState::NoSafePos; }
bool SafeTeleport::Task::isTaskCompleted() const { return mState == TaskState::TaskCompleted; }
bool SafeTeleport::Task::isTaskFailed() const { return mState == TaskState::TaskFailed; }
bool SafeTeleport::Task::isAborted() const { return mAbortFlag.load(); }

SafeTeleport::TaskState SafeTeleport::Task::getState() const { return mState; }

Player* SafeTeleport::Task::getPlayer() const { return mWeakPlayer.tryUnwrap<Player>().as_ptr(); }

void SafeTeleport::Task::updateState(TaskState state) { mState = state; }

void SafeTeleport::Task::updateCounter() { mCounter++; }

void SafeTeleport::Task::sendWaitChunkLoadTip() {
    if (auto player = getPlayer()) {
        mTipPacket.mTitleText = "等待区块加载... ({}/{})"_trf(*player, mCounter, MaxCounter);
        mTipPacket.sendTo(*player);
    }
}

void SafeTeleport::Task::abort() {
    mAbortFlag.store(true);
    updateState(TaskState::TaskFailed);
}

void SafeTeleport::Task::rollback() const {
    if (auto player = getPlayer()) {
        player->teleport(mSourcePos.first, mSourcePos.second);
    }
}

void SafeTeleport::Task::commit() const {
    if (auto player = getPlayer()) {
        player->teleport(mTargetPos.first, mTargetPos.second);
    }
}

bool SafeTeleport::Task::isTargetChunkFullyLoaded() const {
    auto& chunkSource = *mTargetDimension.lock()->mChunkSource.get();
    if (!chunkSource.isWithinWorldLimit(mTargetChunkPos)) return true;
    auto chunk = chunkSource.getOrLoadChunk(mTargetChunkPos, ::ChunkSource::LoadMode::None, true);
    return chunk && static_cast<int>(chunk->mLoadState->load()) >= static_cast<int>(ChunkState::Loaded)
        && !chunk->mIsEmptyClientChunk && chunk->mIsRedstoneLoaded;
}

void SafeTeleport::Task::checkChunkStatus() {
    if (isWaitingChunkLoad()) {
        if (isTargetChunkFullyLoaded()) {
            updateState(TaskState::ChunkLoaded);
        } else if (mCounter > MaxCounter) {
            updateState(TaskState::ChunkLoadTimeout);
        } else {
            updateCounter();
            sendWaitChunkLoadTip();
            teleportToTargetPosAndTryLoadChunk();
        }
    }
}
void SafeTeleport::Task::teleportToTargetPosAndTryLoadChunk() {
    if (auto player = getPlayer()) {
        player->teleport(mTargetPos.first, mTargetPos.second);
    }
}

void SafeTeleport::Task::checkPlayerStatus() {
    if (!getPlayer()) {
        updateState(TaskState::TaskFailed);
    }
}

void SafeTeleport::Task::_tryApplyDimensionFixPatch(DimensionHeightRange const& range) {
    switch (mTargetPos.second) {
    case 1:
        _applyNetherFixPatch(range);
        break;
    default:
        break;
    }
}
void SafeTeleport::Task::_applyNetherFixPatch(DimensionHeightRange const& range) {
    mTargetPos.first.y = range.mMax - 5; // 向下偏移 5 格，避免基岩顶部
}
void SafeTeleport::Task::_findSafePos() {
    static auto const dangerousBlocks =
        std::unordered_set<std::string>{"minecraft:water", "minecraft:lava", "minecraft:fire"};

    auto& targetPos   = mTargetPos.first;
    auto* player      = getPlayer();
    auto& blockSource = *mTargetDimension.lock()->mBlockSource.get();

    auto const& heightRange = player->getDimension().mHeightRange.get();
    auto const  start       = heightRange.mMax;
    auto const  end         = heightRange.mMin;

    Block* headBlock = nullptr; // 头部方块
    Block* legBlock  = nullptr; // 腿部方块

    auto& y = targetPos.y;
    y       = start; // 从最高点开始寻找

    _tryApplyDimensionFixPatch(heightRange); // 尝试应用维度修复补丁

#ifdef DEBUG
    auto& logger = mod::PLand::getInstance().getSelf().getLogger();
#endif

    while (y > end && !mAbortFlag.load()) {
        auto block = &const_cast<Block&>(blockSource.getBlock(targetPos));

        if (!headBlock && !legBlock) { // 第一次循环, 初始化
            headBlock = block;
            legBlock  = block;
        }

#ifdef DEBUG
        logger.debug("[TPR] Y: {}  Block: {}", y, block->getTypeName());
#endif

        if (!block->isAir() &&                                 // 落脚点不是空气
            !dangerousBlocks.contains(block->getTypeName()) && // 落脚点不是危险方块
            headBlock->isAir() &&                              // 头部方块是空气
            legBlock->isAir()                                  // 腿部方块是空气
        ) {
            y++; // 往上一格，当前格为落脚点方块

            updateState(TaskState::FoundSafePos); // 找到安全位置
            return;
        }

        headBlock = legBlock;
        legBlock  = block;
        y--;
    }
    updateState(TaskState::NoSafePos); // 没有找到安全位置
}

void SafeTeleport::Task::launchFindPosTask() {
    ll::coro::keepThis([this]() -> ll::coro::CoroTask<> {
        co_await ll::chrono::ticks(1); // 等待 1_tick 再开始寻找安全位置
        _findSafePos();
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}


SafeTeleport::SafeTeleport() {
    mInterruptableSleep = std::make_shared<ll::coro::InterruptableSleep>();
    mPollingAbortFlag   = std::make_shared<std::atomic_bool>(false);

    ll::coro::keepThis([this, sleep = mInterruptableSleep, abortFlag = mPollingAbortFlag]() -> ll::coro::CoroTask<> {
        while (!abortFlag->load()) {
            co_await sleep->sleepFor(ll::chrono::ticks{10});
            if (abortFlag->load()) break;
            try {
                polling();
            } catch (...) {
                mod::PLand::getInstance().getSelf().getLogger().error(
                    "An exception occurred while polling SafeTeleport tasks"
                );
            }
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

SafeTeleport::~SafeTeleport() {
    mPollingAbortFlag->store(true);
    mInterruptableSleep->interrupt(true);
    for (auto& task : mTasks | std::views::values) {
        task->abort();
    }
    mTasks.clear();
}
SafeTeleport* SafeTeleport::getInstance() { return mod::PLand::getInstance().getSafeTeleport(); }


void SafeTeleport::launchTask(Player& player, DimensionPos targetPos) {
    auto task = std::make_shared<Task>(player, targetPos);
    mTasks.emplace(task->mId, task);
}

void SafeTeleport::polling() {
    auto iter = mTasks.begin();

    while (iter != mTasks.end()) {
        auto& task = iter->second;

        if (!task->isTaskFailed() && !task->isTaskCompleted()) {
            task->checkPlayerStatus(); // 检查玩家是否在线
        }

        switch (task->getState()) {
        case TaskState::Pending:
            handlePending(task);
            break;
        case TaskState::WaitingChunkLoad:
            handleWaitingChunkLoad(task);
            break;
        case TaskState::ChunkLoadTimeout:
            handleChunkLoadTimeout(task);
            break;
        case TaskState::ChunkLoaded:
            handleChunkLoaded(task);
            break;
        case TaskState::FindingSafePos:
            break;
        case TaskState::FoundSafePos:
            handleFoundSafePos(task);
            break;
        case TaskState::NoSafePos:
            handleNoSafePos(task);
            break;
        case TaskState::TaskCompleted:
        case TaskState::TaskFailed:
            iter = mTasks.erase(iter); // 任务完成或失败, 移除任务
            break;
        }

        ++iter;
    }
}

void SafeTeleport::handlePending(SharedTask& task) {
    auto& player = *task->getPlayer();
    mc_utils::sendText(player, "[1/4] 任务已创建"_trf(player));

    if (task->isTargetChunkFullyLoaded()) {
        task->updateState(TaskState::ChunkLoaded);
    } else {
        task->updateState(TaskState::WaitingChunkLoad);
        mc_utils::sendText(player, "[2/4] 目标区块未加载，等待目标区块加载..."_trf(player));
    }
}
void SafeTeleport::handleWaitingChunkLoad(SharedTask& task) { task->checkChunkStatus(); }
void SafeTeleport::handleChunkLoadTimeout(SharedTask& task) {
    auto& player = *task->getPlayer();
    mc_utils::sendText(player, "[2/4] 目标区块加载超时，正在返回原位置..."_trf(player));
    task->rollback();
    task->updateState(TaskState::TaskFailed);
}
void SafeTeleport::handleChunkLoaded(SharedTask& task) {
    auto& player = *task->getPlayer();
    mc_utils::sendText(player, "[3/4] 区块已加载，正在寻找安全位置..."_trf(player));
    task->launchFindPosTask();
    task->updateState(TaskState::FindingSafePos);
}

void SafeTeleport::handleFoundSafePos(SharedTask& task) {
    auto& player = *task->getPlayer();
    mc_utils::sendText(player, "[4/4] 安全位置已找到，正在传送..."_trf(player));
    task->commit();
    task->updateState(TaskState::TaskCompleted);
}
void SafeTeleport::handleNoSafePos(SharedTask& task) {
    auto& player = *task->getPlayer();
    mc_utils::sendText(player, "[3/4] 未找到安全位置，正在返回原位置..."_trf(player));
    task->rollback();
    task->updateState(TaskState::TaskFailed);
}


} // namespace land