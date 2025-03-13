#include "pland/SafeTeleport.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/block/Block.h"
#include "mod/MyMod.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/utils/McUtils.h"
#include <algorithm>
#include <concurrentqueue.h>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>


namespace land {


using DimensionPos = std::pair<Vec3, int>;

enum class TaskState {
    WaitingProcess,              // 等待处理
    WaitingChunkLoad,            // 等待区块加载
    ChunkLoadedWaitingProcess,   // 区块加载完成，等待处理
    FindingSafePos,              // 找安全位置
    FindedSafePosWaitingProcess, // 找到安全位置，等待处理
    TaskCompleted,               // 任务完成
    WaitChunkLoadTimeout,        // 等待区块加载超时
    NoSafePos,                   // 没有安全位置
    PlayerDisconnected,          // 玩家断开连接
};

struct Task {
    uint64_t       id_{};                                         // 任务ID
    Player*        player_{nullptr};                              // 玩家
    UUIDm          uuid_;                                         // 玩家UUID
    DimensionPos   targetPos_;                                    // 目标位置
    DimensionPos   sourcePos_;                                    // 原始位置
    short          scheduleCounter_ = 0;                          // 调度计数器
    TaskState      state_           = TaskState::WaitingProcess;  // 任务状态
    SetTitlePacket action_{SetTitlePacket::TitleType::Actionbar}; // 设置标题包

public:
    Task(Task&)                      = delete;
    Task& operator=(Task&)           = delete;
    Task(Task&&) noexcept            = default;
    Task& operator=(Task&&) noexcept = default;

    explicit Task(uint64_t id, Player* player, DimensionPos targetPos, DimensionPos sourcePos)
    : id_(id),
      player_(player),
      uuid_(player->getUuid()),
      targetPos_(targetPos),
      sourcePos_(sourcePos) {}

    void updateState(TaskState state) { state_ = state; }

    bool operator==(const Task& other) const { return id_ == other.id_; }
};

using TaskPtr = std::shared_ptr<Task>;

constexpr short SCHEDULE_COUNTER_MAX = 64; // 调度计数器最大值


class SafeTeleport::SafeTeleportImpl {
    std::unordered_map<uint64_t, TaskPtr> queue_; // 任务队列
    std::unordered_map<UUIDm, TaskPtr>    queueMap_;
    uint64_t                              idCounter_ = 0; // 唯一ID计数器
    ll::event::ListenerPtr                listener_;      // 玩家退出事件监听器

private:
    inline void findPosImpl(TaskPtr& task) {
        static const std::vector<string> dangerousBlocks = {"minecraft:lava", "minecraft:flowing_lava"};

        auto& logger      = my_mod::MyMod::getInstance().getSelf().getLogger();
        auto& targetPos   = task->targetPos_;
        auto& dimension   = task->player_->getDimension();
        auto& blockSource = task->player_->getDimensionBlockSource();

        short const startY = mc_utils::GetDimensionMaxHeight(dimension); // 维度最大高度
        short const endY   = mc_utils::GetDimensionMinHeight(dimension); // 维度最小高度

        Block* headBlock = nullptr; // 头部方块
        Block* legBlock  = nullptr; // 腿部方块

        BlockPos currentPos{targetPos.first};
        auto&    currentY = currentPos.y;
        currentY          = startY;

        while (currentY > endY) {
            auto& bl = const_cast<Block&>(blockSource.getBlock(currentPos));

            logger.debug(
                "currentY: {}, bl: {}, headBlock: {}, legBlock: {}",
                currentY,
                bl.getTypeName(),
                headBlock ? headBlock->getTypeName() : "nullptr",
                legBlock ? legBlock->getTypeName() : "nullptr"
            );

            if (std::find(dangerousBlocks.begin(), dangerousBlocks.end(), bl.getTypeName()) == dangerousBlocks.end()
                && !bl.isAir() && headBlock->isAir() && legBlock->isAir()) {
                task->targetPos_.first.y = static_cast<float>(currentY) + 1;
                task->updateState(TaskState::FindedSafePosWaitingProcess);
                return;
            }

            if (!headBlock && !legBlock) {
                headBlock = &bl;
                legBlock  = &bl;
            }

            // 交换
            headBlock = legBlock;
            legBlock  = &bl;
            currentY--;
        }

        task->updateState(TaskState::NoSafePos);
    }

    inline void handleWaitingProcess(TaskPtr& task) {
        mc_utils::sendText(*task->player_, "任务已加入队列，请稍后..."_trf(*task->player_));
        if (mc_utils::IsChunkFullLyoaded(task->targetPos_.first, task->player_->getDimensionBlockSource())) {
            task->updateState(TaskState::ChunkLoadedWaitingProcess);
            mc_utils::sendText(*task->player_, "等待区块加载..."_trf(*task->player_));
        } else {
            task->updateState(TaskState::WaitingChunkLoad);
        }
    }

    inline void handleWaitingChunkLoad(TaskPtr& task) {
        auto& target = task->targetPos_;

        Vec3 tmp{target.first};
        tmp.y = 320;
        task->player_->teleport(tmp, target.second); // 防止摔死

        task->action_.mTitleText =
            "等待区块加载... 计数: {}/{}"_trf(*task->player_, task->scheduleCounter_, SCHEDULE_COUNTER_MAX);
        task->action_.sendTo(*task->player_);

        if (task->scheduleCounter_ > SCHEDULE_COUNTER_MAX) {
            task->updateState(TaskState::WaitChunkLoadTimeout);
            return;
        }

        if (mc_utils::IsChunkFullLyoaded(target.first, task->player_->getDimensionBlockSource())) {
            task->updateState(TaskState::ChunkLoadedWaitingProcess);
        } else {
            task->scheduleCounter_++;
        }
    }

    inline void handleChunkLoadedWaitingProcess(TaskPtr& task) {
        mc_utils::sendText(*task->player_, "区块已加载，正在寻找安全位置..."_trf(*task->player_));
        task->updateState(TaskState::FindingSafePos);
        findPosImpl(task);
    }

    inline void handleFindedSafePosWaitingProcess(TaskPtr& task) {
        mc_utils::sendText(*task->player_, "安全位置已找到，正在传送..."_trf(*task->player_));
        task->player_->teleport(task->targetPos_.first, task->targetPos_.second);
        task->updateState(TaskState::TaskCompleted);
    }

    inline void handleFailed(TaskPtr& task, std::string const& reason) {
        mc_utils::sendText<mc_utils::LogLevel::Error>(*task->player_, reason);
        auto& sou = task->sourcePos_;
        task->player_->teleport(sou.first, sou.second);
    }

    inline void processTasks() {
        for (auto iter = queue_.begin(); iter != queue_.end();) {
            auto& task = iter->second;
            try {
                switch (task->state_) {
                case TaskState::WaitingProcess: {
                    handleWaitingProcess(task);
                    break;
                }
                case TaskState::WaitingChunkLoad: {
                    handleWaitingChunkLoad(task);
                    break;
                }
                case TaskState::ChunkLoadedWaitingProcess: {
                    handleChunkLoadedWaitingProcess(task);
                    break;
                }
                case TaskState::FindingSafePos: {
                    break;
                }
                case TaskState::FindedSafePosWaitingProcess: {
                    handleFindedSafePosWaitingProcess(task);
                    break;
                }
                case TaskState::PlayerDisconnected: {
                    iter = queue_.erase(iter);
                    break;
                }
                case TaskState::TaskCompleted: {
                    mc_utils::sendText(*task->player_, "传送成功!"_trf(*task->player_));
                    queueMap_.erase(task->uuid_);
                    iter = queue_.erase(iter);
                    break;
                }
                case TaskState::NoSafePos: {
                    handleFailed(task, "任务失败，未找到安全坐标"_trf(*task->player_));
                    queueMap_.erase(task->uuid_);
                    iter = queue_.erase(iter);
                    break;
                }
                case TaskState::WaitChunkLoadTimeout: {
                    handleFailed(task, "区块加载超时"_trf(*task->player_));
                    queueMap_.erase(task->uuid_);
                    iter = queue_.erase(iter);
                    break;
                }
                }
            } catch (...) {
                queueMap_.erase(task->uuid_);
                iter = queue_.erase(iter);
                my_mod::MyMod::getInstance().getSelf().getLogger().error("传送任务处理失败，任务ID: {}", task->id_);
            }
            ++iter;
        }
    }

    inline void init() {
        ll::coro::keepThis([this]() -> ll::coro::CoroTask<> {
            while (GlobalRepeatCoroTaskRunning.load()) {
                co_await 10_tick;
                if (queue_.empty()) continue;
                processTasks();
            }
            co_return;
        }).launch(ll::thread::ServerThreadExecutor::getDefault());

        listener_ = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
            [this](ll::event::PlayerDisconnectEvent& ev) {
                auto iter = queueMap_.find(ev.self().getUuid());
                if (iter != queueMap_.end()) {
                    iter->second->updateState(TaskState::PlayerDisconnected);
                    queueMap_.erase(iter); // 删除映射
                }
            }
        );
    }

public:
    SafeTeleportImpl(const SafeTeleportImpl&)            = delete;
    SafeTeleportImpl& operator=(const SafeTeleportImpl&) = delete;
    SafeTeleportImpl(SafeTeleportImpl&&)                 = delete;
    SafeTeleportImpl& operator=(SafeTeleportImpl&&)      = delete;

    explicit SafeTeleportImpl() { init(); }

    virtual ~SafeTeleportImpl() {
        if (listener_) ll::event::EventBus::getInstance().removeListener(listener_);
        queue_.clear();    // 清空任务队列
        queueMap_.clear(); // 清空任务映射
    }

    [[nodiscard]] static SafeTeleportImpl& getInstance() {
        static SafeTeleportImpl instance;
        return instance;
    }

    uint64_t createTask(Player* player, DimensionPos targetPos, DimensionPos sourcePos) {
        if (queueMap_.find(player->getRealName()) != queueMap_.end()) {
            mc_utils::sendText<mc_utils::LogLevel::Error>(*player, "传送任务已存在，请等待当前任务完成"_trf(*player));
            return -1;
        }
        auto id   = idCounter_++;
        auto task = std::make_shared<Task>(id, player, targetPos, sourcePos);
        queue_.emplace(id, task);                   // 创建任务
        queueMap_.emplace(player->getUuid(), task); // 创建映射
        return id;
    }
};


void SafeTeleport::teleportTo(Player& player, Vec3 const& pos, int dimid) {
    if (!Config::cfg.land.landTp) {
        mc_utils::sendText<mc_utils::LogLevel::Info>(player, "此功能已被管理员关闭"_trf(player));
        return;
    }

    SafeTeleportImpl::getInstance()
        .createTask(&player, {pos, dimid}, {player.getPosition(), player.getDimensionId().id});
}
SafeTeleport& SafeTeleport::getInstance() {
    static SafeTeleport instance;
    return instance;
}


} // namespace land
