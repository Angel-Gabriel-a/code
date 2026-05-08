# README\.md

# xlog

**A high\-performance asynchronous C\+\+ logging library designed for low\-latency, high\-throughput backend services\.**

Built for engineering practice, featuring lock\-free queue, background writing, size\-based rolling, multi\-sink output and graceful shutdown\.

---

## ✨ Features

- **Lock\-free MPSC Queue** — Multi\-producer single\-consumer, zero lock contention on hot path

- **Fully Asynchronous Logging** — Business threads never block on disk I/O

- **Independent Background Writer Thread**

- **Size\-based Log Rolling** — Auto archive when file reaches specified size

- **Log Retention Policy** — Automatically clean up expired archive logs

- **Multi\-Sink Output**

    - File Sink \(persistent log storage\)

    - Console Sink \(stderr output, optional\)

- **Dual\-level Log Filtering** — Compile\-time strip \+ runtime dynamic level control

- **Backpressure Monitoring** — Count dropped logs when queue is full

- **Graceful Shutdown** — Drain residual queue data to avoid log loss

- **Collision\-free Rolling Naming** — Unique naming for high\-frequency rolling scenarios

---

## 📦 Requirements

- C\+\+17 or higher

- POSIX\-compliant OS \(Linux / macOS\)

- CMake 3\.14\+

---

## 🔧 Build

```bash
mkdir build && cd build
cmake ..
make

```

---

## 🚀 Quick Start

```cpp
#include "xlog/logger.h"
using namespace xlog;

int main() {
    LogConfig cfg;
    cfg.log_dir        = "./logs";
    cfg.base_name      = "my_app";
    cfg.enable_console = true;
    cfg.queue_capacity = 65536;      // Must be power of 2
    cfg.max_file_size  = 64 * 1024 * 1024; // 64MB per log file
    cfg.max_files      = 7;

    // Initialize global singleton logger
    Logger::instance().init(cfg);
    Logger::instance().set_min_level(LogLevel::TRACE);

    // Log examples
    LOG_INFO("server started, port=%d", 8080);
    LOG_DEBUG("connection from %s", "127.0.0.1");
    LOG_WARN("retry attempt %d", 1);
    LOG_ERROR("failed to open file: %s", "config.json");

    // Graceful shutdown
    Logger::instance().shutdown();
    return 0;
}

```

---

## ⚙️ Configuration

|Field|Type|Default|Description|
|---|---|---|---|
|log\_dir|std::string|\./logs|Log file storage directory|
|base\_name|std::string|app|Base name of log files|
|enable\_console|bool|false|Whether mirror logs to stderr|
|queue\_capacity|size\_t|65536|MPSC queue size \(must be power of 2\)|
|max\_file\_size|size\_t|64MB|Threshold for log rolling|
|max\_files|int|7|Maximum number of retained archive logs|
|min\_level|LogLevel|INFO|Minimum printable log level|

---

## 📊 Log Level

Level priority \(from low to high\):

```Plain Text
TRACE < DEBUG < INFO < WARN < ERROR < FATAL

```

- **FATAL**: Print log \-\&gt; invoke `shutdown\(\)` \-\&gt; terminate process by `abort\(\)`

---

## 📁 Log File Naming Rule

### Active Log File

```Plain Text
{log_dir}/{base_name}.log

```

### Archived Rolling Log File

```Plain Text
{log_dir}/{base_name}.{pid}.{YYYYMMDD-HHMMSS}.{seq}.log

```

- **pid**: Current process ID

- **timestamp**: Accurate to seconds

- **seq**: Monotonic incremental sequence \(4 digits\)

- Effectively avoid filename collision under frequent rolling

---

## 🧠 Architecture

```Plain Text
Business Thread: LOG_XXX()
             │
             ▼
       Format LogEntry
             │
             ▼
Lock-free MpscQueue (Multi-Producer)
             │
             ▼
     Background Consumer Thread
             │
             ├─> FileSink::write()    → Rolling + Buffered IO
             └─> ConsoleSink::write() → Stderr Output

```

- Background thread consumes logs continuously

- Periodic flush ensures disk persistence

- Shutdown mechanism drains all residual queue data to prevent loss

---

## 📉 Dropped Log Statistics

When the MPSC queue is full, new logs will be discarded and counted for backpressure analysis\.

```cpp
Logger::instance().shutdown();
uint64_t dropped = Logger::instance().dropped_count();
if (dropped > 0) {
    fprintf(stderr, "warning: %llu log entries dropped\n", dropped);
}

```

**Optimization Suggestions if dropping occurs:**

- Increase `queue\_capacity`

- Reduce high\-frequency debug logs

- Optimize disk I/O pressure

---

## ⚠️ Known Limitations

- Only supports POSIX systems \(Linux / macOS\)

- Single background writing thread, throughput limited by disk I/O

- **No automatic disk\-full recovery**

    - Once disk full is detected, logs fallback to stderr

    - Need to restart process for recovery

- **queue\_capacity must be power of 2**, otherwise throw runtime exception

---

## 🧪 Unit Test

```bash
cd build
ctest --output-on-failure

```

Core test cases:

- `test\_shutdown\_no\_data\_loss` — Verify no log loss during normal shutdown

- `test\_shutdown\_with\_backlog` — Verify dropped \&amp; written statistical correctness

- `test\_rolling\_unique\_names` — Ensure no filename collision in rapid rolling

- `test\_reopen\_size\_tracking` — Persist file size correctly after process restart

---

## 📝 Changelog

### v1\.1\.0

#### 🐛 Bug Fixes

- Fix filename collision when rolling multiple times within one second \(add `roll\_seq\_`\)

- Fix misjudgment of active log file in `scan\_archive\_files\(\)`

- Fix file size not restored after reopening log file

- Fix `initialized\_` flag not reset after shutdown \(support reinitialization\)

- Fix missing condition variable notification causing shutdown stall

---

## 📌 Author

Maintained by Gabriel \| Open source for C\+\+ engineering learning

Suitable for learning high\-performance asynchronous middleware, lock\-free queue, log rolling mechanism\.

> （注：文档部分内容可能由 AI 生成）
