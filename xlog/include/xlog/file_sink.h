#pragma once 
#include "sink.h"
#include <string>
#include <cstdio>
#include <memory>
#include <vector>
#include <ctime>

namespace xlog {

class FileSink : public Sink {
public:
    FileSink(const std::string& log_dir,
             const std::string& base_name,
             size_t max_file_size,
             int max_files);

    ~FileSink() override;

    void write(const char* data, size_t len) override;
    void flush() override;

private:
    void open_file();
    void roll();
    std::string build_current_path() const;
    std::string build_archive_name();                    // 改为非 const，因为需要修改 roll_seq_
    std::vector<std::string> scan_archive_files() const;

private:
    FILE*       fp_{nullptr};
    std::string log_dir_;
    std::string base_name_;
    size_t      max_file_size_;
    int         max_files_;
    size_t      current_size_{0};
    size_t      buffered_{0};
    bool        disk_full_{false};
    uint32_t    roll_seq_{0};                            // 新增：单调递增序号，保证同秒内归档文件名唯一

    static constexpr size_t kFlushThreshold = 4 * 1024;
};

} // namespace xlog