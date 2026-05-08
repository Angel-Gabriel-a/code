#pragma once

#include "log_entry.h"
#include "mpsc_queue.h"
#include "formatter.h"
#include "sink.h"

#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

namespace xlog {

class AsyncWorker {
public:
    explicit AsyncWorker(size_t queue_capacity = 65536);

    ~AsyncWorker();

    // 禁用拷贝
    AsyncWorker(const AsyncWorker&) = delete;
    AsyncWorker& operator=(const AsyncWorker&) = delete;

    // 注册 Sink
    void add_sink(std::unique_ptr<Sink> sink);

    // 提交日志，返回 false 表示队列满或已 shutdown
    bool submit(LogEntry&& entry);

    // 三步 shutdown
    void shutdown();

    // 获取丢弃计数
    uint64_t dropped_count() const;

private:
    void run();

private:
    MpscQueue<LogEntry>                    queue_;
    std::vector<std::unique_ptr<Sink>>     sinks_;
    Formatter                              formatter_;

    std::atomic<bool>  accepting_{true};  // 是否接受新日志
    std::atomic<bool>  stop_{false};      // 通知后台线程退出
    std::thread        thread_;

    std::chrono::steady_clock::time_point last_flush_time_;
    static constexpr auto kFlushInterval = std::chrono::seconds(1);
    static constexpr auto kSleepDuration = std::chrono::milliseconds(1);
};

} // namespace xlog