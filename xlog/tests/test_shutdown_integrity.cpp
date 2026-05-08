// test_shutdown_integrity.cpp
#include "xlog/logger.h"
#include <thread>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <atomic>
#include <cassert>
#include <string>

using namespace xlog;
namespace fs = std::filesystem;

// 统计所有日志文件里包含 marker 的行数
static size_t count_marker_in_logs(const std::string& log_dir,
                                   const std::string& marker) {
    if (!fs::exists(log_dir)) return 0;
    size_t count = 0;
    for (const auto& entry : fs::directory_iterator(log_dir)) {
        if (entry.path().extension() != ".log") continue;
        std::ifstream f(entry.path());
        std::string line;
        while (std::getline(f, line)) {
            if (line.find(marker) != std::string::npos) {
                ++count;
            }
        }
    }
    return count;
}

// 清理测试目录
static void cleanup(const std::string& log_dir) {
    if (fs::exists(log_dir)) {
        fs::remove_all(log_dir);
    }
}

// -------------------------------------------------------
// 测试 1：正常负载下 shutdown 不丢数据
// 预期：文件里的条数 == 成功入队的条数
// -------------------------------------------------------
void test_shutdown_no_data_loss() {
    const std::string log_dir = "./logs_test_shutdown";
    const std::string marker  = "SHUTDOWN_INTEGRITY_MARKER";
    cleanup(log_dir);

    LogConfig cfg;
    cfg.log_dir        = log_dir;
    cfg.base_name      = "shutdown_test";
    cfg.enable_console = false;
    cfg.queue_capacity = 65536;
    cfg.max_file_size  = 1 * 1024 * 1024; // 1MB，避免频繁 rolling 干扰计数
    cfg.max_files      = 10;

    Logger::instance().init(cfg);
    Logger::instance().set_min_level(LogLevel::TRACE);

    const int num_threads    = 4;
    const int logs_per_thread = 2000;
    std::atomic<int> enqueued{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < logs_per_thread; ++j) {
                // LOG_INFO 返回值反映 push 是否成功
                // 如果你的宏没有返回值，可以直接用 Logger::instance().log()
                Logger::instance().log(
                    LogLevel::INFO, __FILE__, __LINE__,
                    "%s thread=%d seq=%d", marker.c_str(), i, j);
                enqueued.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) t.join();

    Logger::instance().shutdown();

    size_t in_files  = count_marker_in_logs(log_dir, marker);
    size_t expected  = static_cast<size_t>(enqueued.load());
    uint64_t dropped = Logger::instance().dropped_count();

    std::cout << "[test_shutdown_no_data_loss]\n"
              << "  enqueued : " << expected  << "\n"
              << "  in files : " << in_files  << "\n"
              << "  dropped  : " << dropped   << "\n";

    // 核心断言：写入文件的条数 == 成功入队的条数
    assert(in_files == expected &&
           "shutdown() 前队列里的数据没有全部写入文件");

    // 辅助断言：dropped 应该为 0（队列足够大）
    assert(dropped == 0 && "队列不应该满");

    std::cout << "  PASS\n\n";
    cleanup(log_dir);
}

// -------------------------------------------------------
// 测试 2：shutdown 在队列积压时调用，数据不能丢
// 模拟方式：用极小队列 + 高频写入，让队列始终有积压
// 预期：(in_files + dropped) == total_attempted
// -------------------------------------------------------
void test_shutdown_with_backlog() {
    const std::string log_dir = "./logs_test_backlog";
    const std::string marker  = "BACKLOG_MARKER";
    cleanup(log_dir);
    fs::create_directories(log_dir);  // 保证目录存在

    LogConfig cfg;
    cfg.log_dir        = log_dir;
    cfg.base_name      = "backlog_test";
    cfg.enable_console = false;
    cfg.queue_capacity = 256;   // 故意很小，制造积压
    cfg.max_file_size  = 1 * 1024 * 1024;
    cfg.max_files      = 10;

    Logger::instance().init(cfg);
    Logger::instance().set_min_level(LogLevel::TRACE);

    const int total = 10000;
    std::atomic<int> attempted{0};

    // 单线程快速写入，让队列来不及消费
    for (int i = 0; i < total; ++i) {
        Logger::instance().log(LogLevel::INFO, __FILE__, __LINE__,
                               "%s seq=%d", marker.c_str(), i);
        attempted.fetch_add(1, std::memory_order_relaxed);
    }

    // 立即 shutdown，此时队列里必然有积压
    Logger::instance().shutdown();

    size_t   in_files = count_marker_in_logs(log_dir, marker);
    uint64_t dropped  = Logger::instance().dropped_count();
    size_t   total_attempted = static_cast<size_t>(attempted.load());

    std::cout << "[test_shutdown_with_backlog]\n"
              << "  attempted : " << total_attempted << "\n"
              << "  in files  : " << in_files        << "\n"
              << "  dropped   : " << dropped         << "\n"
              << "  accounted : " << (in_files + dropped) << "\n";

    // 核心断言：写入文件的 + 队列满丢弃的 == 总尝试写入的
    // 如果这个断言失败，说明 shutdown 时队列里还有数据没被消费就退出了
    assert((in_files + dropped) == total_attempted &&
           "shutdown() 时队列积压数据丢失");

    std::cout << "  PASS\n\n";
    cleanup(log_dir);
}

int main() {
    test_shutdown_no_data_loss();
    test_shutdown_with_backlog();
    std::cout << "All shutdown integrity tests passed.\n";
    return 0;
}