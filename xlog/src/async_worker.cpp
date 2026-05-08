#include "xlog/async_worker.h"
#include <cstring>
#include <chrono>
#include <thread>

namespace xlog {

AsyncWorker::AsyncWorker(size_t queue_capacity)
    : queue_(queue_capacity)
{
    thread_ = std::thread(&AsyncWorker::run, this);
}

AsyncWorker::~AsyncWorker() {
    shutdown();
}

void AsyncWorker::add_sink(std::unique_ptr<Sink> sink) {
    sinks_.emplace_back(std::move(sink));
}

bool AsyncWorker::submit(LogEntry&& entry) {
    if (!accepting_.load(std::memory_order_acquire)) return false;
    return queue_.push(std::move(entry));
}

uint64_t AsyncWorker::dropped_count() const {
    return queue_.dropped_count();
}

void AsyncWorker::shutdown() {
    accepting_.store(false, std::memory_order_release);
    stop_.store(true, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
}

// 消费循环
void AsyncWorker::run() {
    last_flush_time_ = std::chrono::steady_clock::now();
    char buf[600];

    while (true) {
        LogEntry entry;
        if (queue_.pop(entry)) {
            size_t len = formatter_.format(entry, buf, sizeof(buf));
            for (auto& sink : sinks_) {
                sink->write(buf, len);
            }

            auto now = std::chrono::steady_clock::now();
            if (now - last_flush_time_ >= kFlushInterval) {
                for (auto& sink : sinks_) sink->flush();
                last_flush_time_ = now;
            }
        } else {
            if (stop_.load(std::memory_order_acquire)) {
                // 队列已排空，最终 flush
                for (auto& sink : sinks_) sink->flush();
                break;
            }
            std::this_thread::sleep_for(kSleepDuration);
        }
    }
}

} // namespace xlog
