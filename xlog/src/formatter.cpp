#include "xlog/formatter.h"
#include "xlog/log_level.h"
#include <cstdio>
#include <chrono>
#include <ctime>
#include <cstring>
#include <thread>
#include <sstream>

namespace xlog {

size_t Formatter::format_timestamp(
    const std::chrono::system_clock::time_point& tp,
    char* buf,
    size_t buf_size)
{
    using namespace std::chrono;
    auto t = system_clock::to_time_t(tp);
    auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

    std::tm tm_time;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_time, &t);
#else
    localtime_r(&t, &tm_time);
#endif

    return std::snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
        tm_time.tm_year + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec,
        static_cast<long long>(ms.count())
    );
}

const char* Formatter::basename(const char* path) {
    if (!path) return "";
    const char* base1 = std::strrchr(path, '/');
    const char* base2 = std::strrchr(path, '\\');
    const char* base = base1 > base2 ? base1 : base2;
    return base ? (base + 1) : path;
}

size_t Formatter::format(const LogEntry& entry, char* buf, size_t buf_size) {
    char ts_buf[32] = {};
    size_t ts_len = format_timestamp(entry.timestamp, ts_buf, sizeof(ts_buf));

    // 将 thread_id 转为整数
    auto tid_hash = std::hash<std::thread::id>{}(entry.thread_id);

    const char* filename = basename(entry.file);

    int len = std::snprintf(
        buf,
        buf_size,
        "[%s] [%-5s] [tid:%zu] [%s:%d] %.*s\n",
        ts_buf,
        to_string(entry.level),
        tid_hash,
        filename,
        entry.line,
        entry.message_len,
        entry.message
    );

    return len > 0 ? static_cast<size_t>(len) : 0;
}

} // namespace xlog
