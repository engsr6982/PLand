#pragma once
#include "pland/Global.h"

#include "absl/container/flat_hash_map.h"
#include "concurrentqueue.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/thread/ThreadPoolExecutor.h"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace land::internal {

/// 异步落盘任务 (由主线程入队, worker 消费)
struct FlushTask {
    enum class Kind : uint8_t { kLand = 0, kDeleteLand = 1, kAdmins = 2, kBarrier = 3 };

    Kind                                mKind{Kind::kLand};
    LandID                              mId{INVALID_LAND_ID};   // kLand / kDeleteLand
    uint32_t                            mDirtyCount{0};         // kLand: 快照时刻的脏计数
    std::string                         mPayload;               // kLand: 序列化后的 CBOR 载荷
    std::shared_ptr<std::promise<void>> mBarrier;               // kBarrier: 落盘完成信号
};

/**
 * @brief 异步落盘队列: 主线程 O(1) 入队, worker 协程批量消费
 *
 * 设计要点:
 * - 入队时快照: kLand 任务携带调用方序列化好的载荷, worker 只消费载荷,
 *   绝不触碰 Land 对象, 与主线程的 Land 修改无数据竞争
 * - 唤醒合并: 仅在队列从空变为非空时打断一次 sleep, 避免同一 burst 的多次跨线程唤醒
 * - 周期兜底: interrupt 非粘性 (协程不在 sleep 态时调用会被丢弃), 任何丢失的
 *   唤醒在 25ms 内自愈
 * - last-wins 去重: drain 时同一领地的多个任务只保留最后一个; 单生产者 FIFO
 *   保证删除任务排在快照任务之后, 覆盖即删除优先
 * - 落盘失败: 批次重新入队, 等待下一轮 drain 重试
 */
class LandFlushQueue {
    LD_DISABLE_COPY_AND_MOVE(LandFlushQueue);

public:
    /// 消费一批去重后的任务并落盘, 返回是否成功。
    /// 任务所有权留在队列, 失败时由队列重新入队重试, 因此消费方不得移动任务成员。
    using Consumer = std::function<bool(std::vector<FlushTask> const& tasks)>;

    explicit LandFlushQueue(Consumer consumer);
    ~LandFlushQueue();

    /// 主线程: 入队一块领地的序列化快照 (payload 需在调用线程完成序列化)
    void enqueueLand(LandID id, uint32_t dirtyCount, std::string payload);

    /// 主线程: 入队一块领地的删除任务
    void enqueueDelete(LandID id);

    /// 主线程: 通知有 meta 键 (操作员等) 修改, 触发一次批量落盘
    void notifyMetaDirty();

    /// 入队 barrier 并阻塞到 worker 完成本次落盘 (仅快照路径使用)
    void flushAndWait();

    /// 启动 worker 协程
    void start(ll::thread::ThreadPoolExecutor& executor);

    /// 停止 worker 并等待其完成最终落盘 (幂等)
    void stop();

private:
    void notifyWorker();

    /// @return 是否写入了数据 (队列中是否出现过非 barrier 任务)
    bool drainBatch();

    Consumer                               mConsumer;
    moodycamel::ConcurrentQueue<FlushTask> mQueue;
    ll::coro::InterruptableSleep           mInterruptableSleep;
    std::atomic_bool                       mFlushTaskAbort{false};
    std::atomic_bool                       mSignalPending{false};
    std::shared_ptr<std::promise<void>>    mFlushDone;
    absl::flat_hash_map<LandID, FlushTask> mDrainBuffer; // drain 去重缓冲 (worker 独占)
};

} // namespace land::internal
