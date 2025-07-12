#pragma once
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/ecs/WeakEntityRef.h"
#include "pland/Global.h"
#include <cstdint>
#include <ll/api/coro/CoroTask.h>
#include <ll/api/coro/InterruptableSleep.h>
#include <ll/api/thread/ServerThreadExecutor.h>
#include <mc/network/packet/SetTitlePacket.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/ChunkPos.h>
#include <utility>


class DimensionHeightRange;
namespace mce {
class UUID;
}
class ChunkSource;

namespace land {

class SafeTeleport {
public:
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

        void _findSafePos();
        void _tryApplyDimensionFixPatch(DimensionHeightRange const& range); // 尝试应用维度修复补丁
        void _applyNetherFixPatch(DimensionHeightRange const& range);
        friend SafeTeleport;

    public:
        Task(Task const&)            = delete;
        Task& operator=(Task const&) = delete;
        Task(Task&&)                 = delete;
        Task&      operator=(Task&&) = delete;
        LDAPI bool operator==(const Task& other) const;

        LDAPI explicit Task(Player& player, DimensionPos targetPos);

        LDNDAPI bool isPending() const;
        LDNDAPI bool isWaitingChunkLoad() const;
        LDNDAPI bool isChunkLoadTimeout() const;
        LDNDAPI bool isChunkLoaded() const;
        LDNDAPI bool isFindingSafePos() const;
        LDNDAPI bool isFoundSafePos() const;
        LDNDAPI bool isNoSafePos() const;
        LDNDAPI bool isTaskCompleted() const;
        LDNDAPI bool isTaskFailed() const;
        LDNDAPI bool isAborted() const;

        LDNDAPI bool isTargetChunkFullyLoaded() const;

        LDNDAPI TaskState getState() const;

        LDNDAPI Player* getPlayer() const;

        LDAPI void updateState(TaskState state);

        LDAPI void updateCounter();

        LDAPI void sendWaitChunkLoadTip();

        LDAPI void abort();

        LDAPI void rollback() const;

        LDAPI void commit() const;

        LDAPI void checkChunkStatus();                   // 检查目标区块状态
        LDAPI void checkPlayerStatus();                  // 检查玩家是否在线
        LDAPI void teleportToTargetPosAndTryLoadChunk(); // 传送到目标位置并尝试加载区块
        LDAPI void launchFindPosTask();
    };
    using SharedTask = std::shared_ptr<Task>;

    LDAPI explicit SafeTeleport();
    LDAPI ~SafeTeleport();

    LDNDAPI static SafeTeleport* getInstance();

    LDAPI void launchTask(Player& player, DimensionPos targetPos);


private:
    void polling(); // 轮询任务状态

    void handlePending(SharedTask& task);
    void handleWaitingChunkLoad(SharedTask& task);
    void handleChunkLoadTimeout(SharedTask& task);
    void handleChunkLoaded(SharedTask& task);
    void handleFoundSafePos(SharedTask& task);
    void handleNoSafePos(SharedTask& task);

    std::unordered_map<TaskId, SharedTask> mTasks;

    std::shared_ptr<ll::coro::InterruptableSleep> mInterruptableSleep{nullptr};
    std::shared_ptr<std::atomic_bool>             mPollingAbortFlag{nullptr};
};

} // namespace land
