#include "SafeTeleport.h"
#include "pland/Global.h"
#include "pland/PLand.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/McUtils.h"

#include "ll/api/chrono/GameChrono.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include <ll/api/coro/CoroTask.h>
#include <ll/api/coro/InterruptableSleep.h>
#include <ll/api/thread/ServerThreadExecutor.h>
#include <ll/api/thread/ThreadPoolExecutor.h>

#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/player/Player.h"

#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include <mc/deps/core/math/Vec3.h>
#include <mc/deps/game_refs/WeakRef.h>
#include <mc/network/packet/SetTitlePacket.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/ChunkPos.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/Block.h>
#include <mc/world/level/chunk/ChunkSource.h>
#include <mc/world/level/chunk/ChunkState.h>
#include <mc/world/level/chunk/LevelChunk.h>
#include <mc/world/level/dimension/Dimension.h>


#include <cstdint>

namespace land::internal {

using TaskId       = std::uint64_t;
using DimensionPos = std::pair<Vec3, int>;

enum class TaskState {
    // 初始状态
    Pending, // 任务刚创建，等待开始处理

    // 区块加载阶段（轮询检查）
    WaitingChunkLoad, // 等待区块加载
    ChunkLoadTimeout, // 区块加载超时
    ChunkLoaded,      // 区块加载完成

    // 安全位置查找阶段（任务自身协程处理）
    FindingSafePos, // 正在异步查找安全位置
    FoundSafePos,   // 成功找到安全位置
    NoSafePos,      // 未找到安全位置

    // 终止状态
    TaskCompleted, // 任务完成（最终状态）
    TaskFailed     // 任务失败（最终状态）
};

TaskId getNextTaskId() {
    static std::atomic<TaskId> taskId{0};
    return ++taskId;
}

class Task {
    static inline constexpr short MaxCounter = 64;                                  // 最大计数器值
    TaskId const                  mId;                                              // 任务ID
    WeakRef<EntityContext>        mWeakPlayer;                                      // 玩家
    WeakRef<Dimension>            mTargetDimension;                                 // 目标维度
    ChunkPos                      mTargetChunkPos;                                  // 目标区块位置
    DimensionPos const            mSourcePos;                                       // 原位置
    DimensionPos                  mTargetPos;                                       // 目标位置
    TaskState                     mState{TaskState::Pending};                       // 任务状态
    short                         mCounter{0};                                      // 计数器
    SetTitlePacket                mTipPacket{SetTitlePacket::TitleType::Actionbar}; // 提示包
    std::atomic<bool>             mAbortFlag{false};                                // 终止标志

    void _findSafePos() {
        static auto const dangerousBlocks =
            std::unordered_set<std::string>{"minecraft:water", "minecraft:lava", "minecraft:fire"};

        auto& targetPos = mTargetPos.first;
        auto  player    = getPlayer();
        if (!player) {
            updateState(TaskState::TaskFailed);
            return;
        }

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
        auto& logger = land::PLand::getInstance().getSelf().getLogger();
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

            if (
                !block->isAir() &&                                 // 落脚点不是空气
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
    void _tryApplyDimensionFixPatch(DimensionHeightRange const& range) {
        switch (mTargetPos.second) {
        case 1:
            _applyNetherFixPatch(range);
            break;
        default:
            break;
        }
    }
    void _applyNetherFixPatch(DimensionHeightRange const& range) {
        mTargetPos.first.y = range.mMax - 5; // 向下偏移 5 格，避免基岩顶部
    }
    friend SafeTeleport;

public:
    LD_DISABLE_COPY_AND_MOVE(Task);

    bool operator==(const Task& other) const { return mId == other.mId; }

    explicit Task() = delete;
    explicit Task(Player& player, DimensionPos targetPos)
    : mId(getNextTaskId()),
      mWeakPlayer(player.getEntityContext().getWeakRef()),
      mTargetDimension(player.getLevel().getDimension(targetPos.second)),
      mTargetChunkPos(ChunkPos(targetPos.first)),
      mSourcePos({player.getPosition(), player.getDimensionId()}),
      mTargetPos(targetPos) {
        mTargetPos.first.x += 0.5; // 方块中心
        mTargetPos.first.z += 0.5;
        mTargetPos.first.y  = 3389;
    }

    bool isPending() const { return mState == TaskState::Pending; }
    bool isWaitingChunkLoad() const { return mState == TaskState::WaitingChunkLoad; }
    bool isChunkLoadTimeout() const { return mState == TaskState::ChunkLoadTimeout; }
    bool isChunkLoaded() const { return mState == TaskState::ChunkLoaded; }
    bool isFindingSafePos() const { return mState == TaskState::FindingSafePos; }
    bool isFoundSafePos() const { return mState == TaskState::FoundSafePos; }
    bool isNoSafePos() const { return mState == TaskState::NoSafePos; }
    bool isTaskCompleted() const { return mState == TaskState::TaskCompleted; }
    bool isTaskFailed() const { return mState == TaskState::TaskFailed; }
    bool isAborted() const { return mAbortFlag.load(); }

    TaskState getState() const { return mState; }

    optional_ref<Player> getPlayer() const {
        auto player = mWeakPlayer.tryUnwrap<Mob>();
        if (!player || player->getEntityTypeId() != ActorType::Player) {
            return nullptr;
        }
        return static_cast<Player*>(player.as_ptr());
    }

    void updateState(TaskState state) { mState = state; }

    void updateCounter() { mCounter++; }

    void sendWaitChunkLoadTip() {
        if (auto player = getPlayer()) {
            mTipPacket.mTitleText = "等待区块加载... ({}/{})"_trl(player->getLocaleCode(), mCounter, MaxCounter);
            mTipPacket.sendTo(*player);
        }
    }

    void abort() {
        mAbortFlag.store(true);
        updateState(TaskState::TaskFailed);
    }

    void rollback() const {
        if (auto player = getPlayer()) {
            player->teleport(mSourcePos.first, mSourcePos.second);
        }
    }

    void commit() const {
        if (auto player = getPlayer()) {
            player->teleport(mTargetPos.first, mTargetPos.second);
        }
    }

    bool isTargetChunkFullyLoaded() const {
        auto dim = mTargetDimension.lock();
        if (!dim) {
            return false;
        }
        auto& chunkSource = dim->getChunkSource();
        if (!chunkSource.isWithinWorldLimit(mTargetChunkPos)) return true;
        auto chunk = chunkSource.getOrLoadChunk(mTargetChunkPos, ::ChunkSource::LoadMode::None, true);
        return chunk && static_cast<int>(chunk->mLoadState->load()) >= static_cast<int>(ChunkState::Loaded)
            && !chunk->mIsEmptyClientChunk && chunk->mIsRedstoneLoaded;
    }

    void checkChunkStatus() {
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
    void teleportToTargetPosAndTryLoadChunk() {
        if (auto player = getPlayer()) {
            player->teleport(mTargetPos.first, mTargetPos.second);
        }
    }

    void checkPlayerStatus() {
        if (!getPlayer()) {
            updateState(TaskState::TaskFailed);
        }
    }

    void launchFindPosTask() {
        ll::coro::keepThis([this]() -> ll::coro::CoroTask<> {
            co_await ll::chrono::ticks(1); // 等待 1_tick 再开始寻找安全位置
            _findSafePos();
            co_return;
        }).launch(ll::thread::ServerThreadExecutor::getDefault());
    }
};
using SharedTask = std::shared_ptr<Task>;


struct SafeTeleport::Impl {
    std::unordered_map<TaskId, SharedTask> mTasks;

    std::shared_ptr<ll::coro::InterruptableSleep> mInterruptableSleep{nullptr};
    std::shared_ptr<std::atomic_bool>             mPollingAbortFlag{nullptr};


    void polling() {
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

    void handlePending(SharedTask& task) {
        auto& player = *task->getPlayer();
        feedback_utils::sendText(player, "[1/4] 任务已创建"_trl(player.getLocaleCode()));

        if (task->isTargetChunkFullyLoaded()) {
            task->updateState(TaskState::ChunkLoaded);
        } else {
            task->updateState(TaskState::WaitingChunkLoad);
            feedback_utils::sendText(player, "[2/4] 目标区块未加载，等待目标区块加载..."_trl(player.getLocaleCode()));
        }
    }
    void handleWaitingChunkLoad(SharedTask& task) { task->checkChunkStatus(); }
    void handleChunkLoadTimeout(SharedTask& task) {
        auto& player = *task->getPlayer();
        feedback_utils::sendText(player, "[2/4] 目标区块加载超时，正在返回原位置..."_trl(player.getLocaleCode()));
        task->rollback();
        task->updateState(TaskState::TaskFailed);
    }
    void handleChunkLoaded(SharedTask& task) {
        auto& player = *task->getPlayer();
        feedback_utils::sendText(player, "[3/4] 区块已加载，正在寻找安全位置..."_trl(player.getLocaleCode()));
        task->launchFindPosTask();
        task->updateState(TaskState::FindingSafePos);
    }
    void handleFoundSafePos(SharedTask& task) {
        auto& player = *task->getPlayer();
        feedback_utils::sendText(player, "[4/4] 安全位置已找到，正在传送..."_trl(player.getLocaleCode()));
        task->commit();
        task->updateState(TaskState::TaskCompleted);
    }
    void handleNoSafePos(SharedTask& task) {
        auto& player = *task->getPlayer();
        feedback_utils::sendText(player, "[3/4] 未找到安全位置，正在返回原位置..."_trl(player.getLocaleCode()));
        task->rollback();
        task->updateState(TaskState::TaskFailed);
    }
};

SafeTeleport::SafeTeleport() : impl(std::make_unique<Impl>()) {
    impl->mInterruptableSleep = std::make_shared<ll::coro::InterruptableSleep>();
    impl->mPollingAbortFlag   = std::make_shared<std::atomic_bool>(false);

    ll::coro::keepThis(
        [this, sleep = impl->mInterruptableSleep, abortFlag = impl->mPollingAbortFlag]() -> ll::coro::CoroTask<> {
            while (!abortFlag->load()) {
                co_await sleep->sleepFor(ll::chrono::ticks{10});
                if (abortFlag->load()) break;
                try {
                    impl->polling();
                } catch (...) {
                    land::PLand::getInstance().getSelf().getLogger().error(
                        "An exception occurred while polling SafeTeleport tasks"
                    );
                }
            }
            co_return;
        }
    ).launch(ll::thread::ServerThreadExecutor::getDefault());
}

SafeTeleport::~SafeTeleport() {
    impl->mPollingAbortFlag->store(true);
    impl->mInterruptableSleep->interrupt(true);
    for (auto& task : impl->mTasks | std::views::values) {
        task->abort();
    }
    impl->mTasks.clear();
}

void SafeTeleport::launchTask(Player& player, BlockPos const& pos, DimensionType dimension) {
    // auto task = std::make_shared<Task>(player, {pos, dimension});
    auto task = SharedTask{
        new Task{player, {pos, dimension}}
    };
    impl->mTasks.emplace(task->mId, task);
}


} // namespace land::internal
