#include "xlog/logger.h"
#include "xlog/formatter.h"
#include <cstdio>
#include <cstdarg>

namespace xlog {

// Meyers Singleton
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::init(const LogConfig& config) {
    bool expected = false;
    if (!initialized_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return; // 已初始化，静默忽略
    }

    min_level_.store(config.min_level, std::memory_order_relaxed);

    worker_ = std::make_unique<AsyncWorker>(config.queue_capacity);

    // 注册 FileSink
    worker_->add_sink(std::make_unique<FileSink>(
        config.log_dir,
        config.base_name,
        config.max_file_size,
        config.max_files
    ));

    // 可选 ConsoleSink
    if (config.enable_console) {
        worker_->add_sink(std::make_unique<ConsoleSink>());
    }
}

void Logger::set_min_level(LogLevel level) {
    min_level_.store(level, std::memory_order_relaxed);
}

uint64_t Logger::dropped_count() const {
    if (worker_) return worker_->dropped_count();
    return 0;
}

void Logger::shutdown() {
    if (!initialized_.load(std::memory_order_acquire)) return;
    if (shutdown_.exchange(true, std::memory_order_acq_rel)) return;

    if (worker_) worker_->shutdown();

    uint64_t dropped = dropped_count();
    if (dropped > 0) {
        std::fprintf(stderr, "[xlog] dropped %llu log entries\n",
                     static_cast<unsigned long long>(dropped));
    }

        // 重置状态，允许重新 init()
    shutdown_.store(false, std::memory_order_release);
    initialized_.store(false, std::memory_order_release);
}

void Logger::log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    // 运行期级别过滤
    if (level < min_level_.load(std::memory_order_relaxed)) return;

    // init 前调用，写 stderr
    if (!initialized_.load(std::memory_order_acquire)) {
        char buf[512];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        std::fprintf(stderr, "[xlog not initialized] %s\n", buf);
        return;
    }

    // shutdown 后调用，丢弃
    if (shutdown_.load(std::memory_order_acquire)) return;

    // 栈上构造 LogEntry
    LogEntry entry;
    entry.level = level;
    entry.timestamp = std::chrono::system_clock::now();
    entry.thread_id = std::this_thread::get_id();
    entry.file = file;
    entry.line = line;

    va_list args;
    va_start(args, fmt);
    entry.message_len = std::vsnprintf(entry.message, sizeof(entry.message), fmt, args);
    va_end(args);

    if (worker_) {
        worker_->submit(std::move(entry)); // 失败静默处理
    }
}

} // namespace xlog
