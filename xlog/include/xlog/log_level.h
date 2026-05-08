#pragma once

#include <cstddef>

namespace xlog {

// 日志级别枚举
enum class LogLevel : int {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// 返回固定宽度字符串（左对齐 5 个字符）
inline const char* to_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKN ";
    }
}

} // namespace xlog