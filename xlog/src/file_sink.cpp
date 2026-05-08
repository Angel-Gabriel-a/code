#include "xlog/file_sink.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <iostream>

namespace xlog {

FileSink::FileSink(const std::string& log_dir,
                   const std::string& base_name,
                   size_t max_file_size,
                   int max_files)
    : log_dir_(log_dir),
      base_name_(base_name),
      max_file_size_(max_file_size),
      max_files_(max_files)
{
    // 自动创建目录
#if defined(_WIN32) || defined(_WIN64)
    _mkdir(log_dir_.c_str());
#else
    mkdir(log_dir_.c_str(), 0755);
#endif

    open_file();
}


FileSink::~FileSink() {
    flush();
    if (fp_) fclose(fp_);
}

// 构建当前写入文件路径
std::string FileSink::build_current_path() const {
    pid_t pid = getpid();
    return log_dir_ + "/" + base_name_ + "." + std::to_string(pid) + ".log";
}

// 构建归档名
// 1. build_archive_name：加入零填充序号，签名去掉 const
std::string FileSink::build_archive_name() {
    time_t t = std::time(nullptr);
    char ts[32] = {};
    std::tm tm_time;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_time, &t);
#else
    localtime_r(&t, &tm_time);
#endif
    std::snprintf(ts, sizeof(ts), "%04d%02d%02d-%02d%02d%02d",
                  tm_time.tm_year + 1900,
                  tm_time.tm_mon + 1,
                  tm_time.tm_mday,
                  tm_time.tm_hour,
                  tm_time.tm_min,
                  tm_time.tm_sec);

    char seq_buf[8] = {};
    std::snprintf(seq_buf, sizeof(seq_buf), "%04u", roll_seq_++);

    pid_t pid = getpid();
    return log_dir_ + "/" + base_name_ + "." + std::to_string(pid)
           + "." + ts + "." + seq_buf + ".log";
}

// 2. open_file：用 ftell 初始化 current_size_，避免追加模式下大小从 0 开始
void FileSink::open_file() {
    std::string path = build_current_path();
    fp_ = std::fopen(path.c_str(), "ab");
    if (!fp_) {
        std::fprintf(stderr, "[xlog] Failed to open log file %s: %s\n",
                     path.c_str(), std::strerror(errno));
        disk_full_ = true;
    } else {
        chmod(path.c_str(), 0644);
        if (std::fseek(fp_, 0, SEEK_END) == 0) {
            long size = std::ftell(fp_);
            current_size_ = (size > 0) ? static_cast<size_t>(size) : 0;
        } else {
            current_size_ = 0;
        }
        buffered_ = 0;
        disk_full_ = false;
    }
}

// 3. scan_archive_files：用裸文件名比较，排除当前活跃文件
std::vector<std::string> FileSink::scan_archive_files() const {
    std::vector<std::string> archives;
    DIR* dir = opendir(log_dir_.c_str());
    if (!dir) return archives;

    std::string current_full = build_current_path();
    std::string current_name = current_full.substr(current_full.rfind('/') + 1);

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.find(base_name_) == 0 && name != current_name) {
            archives.push_back(log_dir_ + "/" + name);
        }
    }
    closedir(dir);

    std::sort(archives.begin(), archives.end());
    return archives;
}


// 写入数据
void FileSink::write(const char* data, size_t len) {
    if (disk_full_) {
        std::fwrite(data, 1, len, stderr);
        return;
    }

    size_t written = std::fwrite(data, 1, len, fp_);
    if (written < len) {
        disk_full_ = true;
        std::fprintf(stderr, "[xlog] disk full, falling back to stderr\n");
        return;
    }

    current_size_ += written;
    buffered_ += written;

    if (buffered_ >= kFlushThreshold) flush();

    if (current_size_ >= max_file_size_) roll();
}

// flush
void FileSink::flush() {
    if (fp_) {
        std::fflush(fp_);
        buffered_ = 0;
    }
}

// roll 文件
void FileSink::roll() {
    flush();
    if (!fp_) return;

    std::string current_path = build_current_path();
    std::string archive_path = build_archive_name();

    fclose(fp_);
    fp_ = nullptr;

    if (std::rename(current_path.c_str(), archive_path.c_str()) != 0) {
        std::fprintf(stderr, "[xlog] Failed to rename log file: %s\n", std::strerror(errno));
    }

    // 删除超出 max_files_ 的最老归档
    auto archives = scan_archive_files();
    while (archives.size() > static_cast<size_t>(max_files_)) {
        std::remove(archives.front().c_str());
        archives.erase(archives.begin());
    }

    open_file();
}

} // namespace xlog
