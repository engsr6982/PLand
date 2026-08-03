#include "LandFlushQueue.h"

#include "ll/api/coro/CoroTask.h"

#include "pland/PLand.h"

namespace land::internal {

namespace {
constexpr auto kFlushInterval = std::chrono::milliseconds(25);
}

LandFlushQueue::LandFlushQueue(Consumer consumer) : mConsumer(std::move(consumer)) {}

LandFlushQueue::~LandFlushQueue() { stop(); }

void LandFlushQueue::enqueueLand(LandID id, uint32_t dirtyCount, std::string payload) {
    mQueue.enqueue(FlushTask{FlushTask::Kind::kLand, id, dirtyCount, std::move(payload), {}});
    notifyWorker();
}

void LandFlushQueue::enqueueDelete(LandID id) {
    mQueue.enqueue(FlushTask{FlushTask::Kind::kDeleteLand, id, 0, {}, {}});
    notifyWorker();
}

void LandFlushQueue::notifyMetaDirty() {
    // kAdmins 任务仅作为"有 meta 键修改"的工作标记, 消费端见批次即无条件写全部 meta 键
    mQueue.enqueue(FlushTask{FlushTask::Kind::kAdmins, INVALID_LAND_ID, 0, {}, {}});
    notifyWorker();
}

void LandFlushQueue::flushAndWait() {
    auto barrier = std::make_shared<std::promise<void>>();
    mQueue.enqueue(FlushTask{FlushTask::Kind::kBarrier, INVALID_LAND_ID, 0, {}, barrier});
    mInterruptableSleep.interrupt(false); // 总是尝试唤醒, 让 worker 尽快完成本次 drain
    barrier->get_future().wait();
}

void LandFlushQueue::start(ll::thread::ThreadPoolExecutor& executor) {
    mFlushDone = std::make_shared<std::promise<void>>();
    auto done  = mFlushDone;
    ll::coro::keepThis([this, done]() -> ll::coro::CoroTask<> {
        while (!mFlushTaskAbort.load(std::memory_order_relaxed)) {
            if (!drainBatch()) {
                // 唤醒合并: 清标志后复查一次队列, 闭合生产者竞态窗口
                mSignalPending.store(false, std::memory_order_relaxed);
                if (mQueue.size_approx() > 0) {
                    continue;
                }
                // 周期兜底: 丢失的唤醒在 25ms 内自愈
                co_await mInterruptableSleep.sleepFor(kFlushInterval);
            }
        }
        // 关服: 清空剩余队列并最终落盘
        // (stop 用 interrupt(true) 会使此处运行在主线程, 快照载荷不读 Land, 同样安全)
        (void)drainBatch();
        done->set_value();
        co_return;
    }).launch(executor);
}

void LandFlushQueue::stop() {
    mFlushTaskAbort.store(true);
    mInterruptableSleep.interrupt(true); // 在主线程原地 resume, 完成最终 drain
    if (mFlushDone) {
        mFlushDone->get_future().wait(); // 等待 worker 完全退出 (含最终落盘), 避免 UAF
    }
}

void LandFlushQueue::notifyWorker() {
    // 唤醒合并: 仅在队列从空变为非空时打断一次
    // 使用 interrupt(false) 转移到线程池执行, 避免在主线程原地 resume 阻塞 server 线程
    if (!mSignalPending.exchange(true, std::memory_order_relaxed)) {
        mInterruptableSleep.interrupt(false);
    }
}

bool LandFlushQueue::drainBatch() {
    mDrainBuffer.clear();
    FlushTask task;
    std::vector<std::shared_ptr<std::promise<void>>> barriers;
    bool hasWork = false;

    while (mQueue.try_dequeue(task)) {
        hasWork = true;
        switch (task.mKind) {
        case FlushTask::Kind::kLand:
        case FlushTask::Kind::kDeleteLand:
            // last-wins 去重: 同一领地的多个任务只保留最后一个。
            // 单生产者 FIFO 保证删除任务必定排在快照任务之后, 覆盖即删除优先。
            mDrainBuffer[task.mId] = std::move(task);
            break;
        case FlushTask::Kind::kBarrier:
            barriers.push_back(std::move(task.mBarrier));
            break;
        default:
            break; // kAdmins: 无需状态, 仅触发一次批量写
        }
    }

    if (!hasWork) {
        return false; // 队列为空, 无事可做
    }

    std::vector<FlushTask> batch;
    batch.reserve(mDrainBuffer.size());
    for (auto& [id, t] : mDrainBuffer) {
        batch.push_back(std::move(t));
    }

    if (!mConsumer(batch)) {
        // 落盘失败: 重新入队, 等待下一轮 drain 重试
        PLand::getInstance().getSelf().getLogger().error("Failed to write land batch to database, retrying...");
        for (auto& t : batch) {
            mQueue.enqueue(std::move(t));
        }
    }

    for (auto& barrier : barriers) {
        barrier->set_value();
    }
    return true;
}

} // namespace land::internal
