#pragma once

#include "log_entry.h"
#include <cstddef>

namespace xlog {

class Formatter {
public:
    // 格式化 LogEntry 到 buf，返回实际写入长度
    // buf 建议大小 600 字节
    size_t format(const LogEntry& entry, char* buf, size_t buf_size);

private:
    // 格式化时间戳到毫秒，返回写入长度
    size_t format_timestamp(
        const std::chrono::system_clock::time_point& tp,
        char* buf,
        size_t buf_size
    );
    
    // 提取文件名（去掉路径前缀）
    const char* basename(const char* path);
};

} // namespace xlog