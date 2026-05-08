#pragma once

#include "async_worker.h"
#include "file_sink.h"
#include "console_sink.h"
#include "log_level.h"

#include <string>
#include <memory>
#include <atomic>
#include <cstdarg>

namespace xlog {

struct LogConfig {
    std::string log_dir        = "./logs";
    std::string base_name      = "app";
    LogLevel    min_level      = LogLevel::INFO;
    size_t      max_file_size  = 100 * 1024 * 1024;  // 100MB
    int         max_files      = 5;
    size_t      queue_capacity = 65536;
    bool        enable_console = false;
};

class Logger {
public:
    static Logger& instance();

    void init(const LogConfig& config);
    void log(LogLevel level, const char* file, int line, const char* fmt, ...);
    void shutdown();
    void set_min_level(LogLevel level);
    uint64_t dropped_count() const;

private:
    Logger() = default;
    ~Logger() = default;

    std::atomic<bool>            initialized_{false};
    std::atomic<bool>            shutdown_{false};
    std::atomic<LogLevel>        min_level_{LogLevel::INFO};
    std::unique_ptr<AsyncWorker> worker_;
};

} // namespace xlog

#ifndef XLOG_MIN_LEVEL
#define XLOG_MIN_LEVEL 0
#endif

#define XLOG_LEVEL_TRACE 0
#define XLOG_LEVEL_DEBUG 1
#define XLOG_LEVEL_INFO  2
#define XLOG_LEVEL_WARN  3
#define XLOG_LEVEL_ERROR 4
#define XLOG_LEVEL_FATAL 5

#define _XLOG_LOG(level, fmt, ...) \
    ::xlog::Logger::instance().log(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#if XLOG_MIN_LEVEL <= XLOG_LEVEL_TRACE
#define LOG_TRACE(fmt, ...) _XLOG_LOG(::xlog::LogLevel::TRACE, fmt, ##__VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...) do {} while(0)
#endif

#if XLOG_MIN_LEVEL <= XLOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) _XLOG_LOG(::xlog::LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) do {} while(0)
#endif

#if XLOG_MIN_LEVEL <= XLOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) _XLOG_LOG(::xlog::LogLevel::INFO, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) do {} while(0)
#endif

#if XLOG_MIN_LEVEL <= XLOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) _XLOG_LOG(::xlog::LogLevel::WARN, fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) do {} while(0)
#endif

#if XLOG_MIN_LEVEL <= XLOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) _XLOG_LOG(::xlog::LogLevel::ERROR, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) do {} while(0)
#endif

#define LOG_FATAL(fmt, ...) \
    do { \
        _XLOG_LOG(::xlog::LogLevel::FATAL, fmt, ##__VA_ARGS__); \
        ::xlog::Logger::instance().shutdown(); \
        std::abort(); \
    } while(0)
