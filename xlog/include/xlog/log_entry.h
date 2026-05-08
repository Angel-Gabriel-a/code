#pragma once

#include "log_level.h"
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace xlog {

// 对齐到 512 字节
struct alignas(512) LogEntry {
    LogLevel                              level;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id                       thread_id;
    const char*                           file;         // __FILE__ 指针
    int                                   line;
    int                                   message_len;  // vsnprintf 写入长度
    char                                  message[464];

    // 构造函数，默认初始化
    LogEntry()
        : level(LogLevel::INFO),
          timestamp(std::chrono::system_clock::now()),
          thread_id(),
          file(nullptr),
          line(0),
          message_len(0) 
    {
        std::memset(message, 0, sizeof(message));
    }

    // 禁用拷贝构造，允许移动
    LogEntry(const LogEntry&) = delete;
    LogEntry& operator=(const LogEntry&) = delete;

    LogEntry(LogEntry&&) = default;
    LogEntry& operator=(LogEntry&&) = default;
};

static_assert(sizeof(LogEntry) == 512, "LogEntry size must be 512 bytes");

} // namespace xlog
