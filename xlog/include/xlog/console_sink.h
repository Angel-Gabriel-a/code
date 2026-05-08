#pragma once

#include "sink.h"
#include <cstdio>

namespace xlog {

class ConsoleSink : public Sink {
public:
    void write(const char* data, size_t len) override {
        std::fwrite(data, 1, len, stderr);
    }

    void flush() override {
        std::fflush(stderr);
    }
};

} // namespace xlog