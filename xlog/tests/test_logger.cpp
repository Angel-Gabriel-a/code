#include "xlog/logger.h"
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

using namespace xlog;

// 每个线程写 N 条日志
void worker_thread(int id, int count) {
    for (int i = 0; i < count; ++i) {
        LOG_INFO("Thread %d logging %d", id, i);
        LOG_DEBUG("Thread %d debug %d", id, i);
    }
}
/*
int main() {
    // 初始化 Logger
    LogConfig cfg;
    cfg.log_dir = "./logs";
    cfg.base_name = "test_app";
    cfg.enable_console = true;     // 同时输出到 stderr
    cfg.queue_capacity = 65536;     // 测试用小队列可触发丢弃
    cfg.max_file_size = 1024 * 1024; // 1MB 小文件用于测试 rolling
    cfg.max_files = 20;

    Logger::instance().init(cfg);
    Logger::instance().set_min_level(LogLevel::TRACE);

    const int num_threads = 4;
    const int logs_per_thread = 1000;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread, i, logs_per_thread);
    }

    for (auto& t : threads) t.join();

    // 主线程写一条 FATAL 测试（会触发 shutdown + abort）
    // LOG_FATAL("Fatal test, process aborting");

    // 普通 shutdown
    Logger::instance().shutdown();

    // 输出丢弃计数
    uint64_t dropped = Logger::instance().dropped_count();
    std::cout << "Total dropped log entries: " << dropped << std::endl;

    return 0;
}
*/