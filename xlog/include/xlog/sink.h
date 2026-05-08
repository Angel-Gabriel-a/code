#pragma once

#include <cstddef>

namespace xlog {

class Sink {
public:
    virtual ~Sink() = default;

    // 写入日志数据
    // data: 指向日志字符缓冲区
    // len: 写入长度
    virtual void write(const char* data, size_t len) = 0;

    // flush 缓冲区
    virtual void flush() = 0;
};

} // namespace xlog