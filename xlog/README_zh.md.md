# README\_zh\.md

# xlog

**一款面向低延迟、高吞吐后端服务的高性能异步 C\+\+ 日志库**

本项目为工程实践级日志库，具备无锁队列、后台异步写入、按大小滚动、多输出终端、优雅关闭等核心能力。

---

## ✨ 功能特性

- **无锁 MPSC 队列** — 多生产者单消费者，业务路径零锁竞争

- **纯异步日志** — 业务线程不会阻塞在磁盘IO操作上

- **独立后台写线程**

- **按文件大小滚动日志** — 文件达到阈值自动归档拆分

- **日志保留策略** — 自动清理过期归档日志

- **多输出终端（Sink）**

    - 文件终端（持久化日志）

    - 控制台终端（标准错误输出，可选开启）

- **双层日志过滤** — 编译期剔除 \+ 运行时动态级别控制

- **背压监控** — 队列满时统计丢弃日志数量

- **优雅关闭** — 清空队列残留数据，防止日志丢失

- **防冲突滚动命名** — 高频滚动场景下保证日志文件名唯一

---

## 📦 环境依赖

- C\+\+17 及以上

- 兼容 POSIX 操作系统（Linux / macOS）

- CMake 3\.14\+

---

## 🔧 编译方式

```bash
mkdir build && cd build
cmake ..
make

```

---

## 🚀 快速上手

```cpp
#include "xlog/logger.h"
using namespace xlog;

int main() {
    LogConfig cfg;
    cfg.log_dir        = "./logs";
    cfg.base_name      = "my_app";
    cfg.enable_console = true;
    cfg.queue_capacity = 65536;      // 必须为2的幂次方
    cfg.max_file_size  = 64 * 1024 * 1024; // 单个日志文件最大64MB
    cfg.max_files      = 7;

    // 初始化全局单例日志器
    Logger::instance().init(cfg);
    Logger::instance().set_min_level(LogLevel::TRACE);

    // 日志使用示例
    LOG_INFO("服务启动，端口=%d", 8080);
    LOG_DEBUG("客户端连接地址：%s", "127.0.0.1");
    LOG_WARN("重试次数：%d", 1);
    LOG_ERROR("文件打开失败：%s", "config.json");

    // 优雅关闭日志系统
    Logger::instance().shutdown();
    return 0;
}

```

---

## ⚙️ 配置参数

|参数名|类型|默认值|参数说明|
|---|---|---|---|
|log\_dir|std::string|\./logs|日志文件存储目录|
|base\_name|std::string|app|日志文件基础名称|
|enable\_console|bool|false|是否同时输出到控制台（标准错误）|
|queue\_capacity|size\_t|65536|无锁队列容量（必须为2的幂次方）|
|max\_file\_size|size\_t|64MB|触发日志滚动的文件大小阈值|
|max\_files|int|7|最大保留归档日志文件数量|
|min\_level|LogLevel|INFO|最低打印日志级别|

---

## 📊 日志级别

级别优先级（由低到高）：

```Plain Text
TRACE < DEBUG < INFO < WARN < ERROR < FATAL

```

- **FATAL（致命错误）**：打印日志 \-\&gt; 关闭日志系统 \-\&gt; 终止程序运行

---

## 📁 日志文件命名规则

### 正在写入的活跃日志

```Plain Text
{log_dir}/{base_name}.log

```

### 滚动归档后的历史日志

```Plain Text
{log_dir}/{base_name}.{pid}.{YYYYMMDD-HHMMSS}.{seq}.log

```

- **pid**：当前进程ID

- **timestamp**：精确到秒的时间戳

- **seq**：自增序列号（固定4位）

- 有效解决短时间内多次滚动导致的文件名冲突问题

---

## 🧠 架构原理

```Plain Text
业务线程：LOG_XXX()
             │
             ▼
        格式化日志实体
             │
             ▼
        无锁MPSC队列（多生产者入队）
             │
             ▼
        后台消费线程（单线程）
             │
             ├─> 文件输出终端 → 日志滚动 + 缓冲区写入
             └─> 控制台输出终端 → 标准错误打印

```

- 后台线程持续消费队列中的日志数据

- 定时刷盘，保证日志持久化落盘

- 关闭流程会清空队列残留日志，避免数据丢失

---

## 📉 日志丢弃统计

当无锁队列已满时，新增日志会被丢弃并计数，用于分析系统背压情况。

```cpp
Logger::instance().shutdown();
uint64_t dropped = Logger::instance().dropped_count();
if (dropped > 0) {
    fprintf(stderr, "警告：丢弃日志条数：%llu\n", dropped);
}

```

**出现日志丢弃的优化方案：**

- 调大队列容量 queue\_capacity

- 减少高频打印的调试日志

- 优化磁盘IO压力，提升写入速度

---

## ⚠️ 已知局限性

- 仅支持 POSIX 系列操作系统（Linux / macOS）

- 单后台写线程，吞吐量受磁盘IO性能限制

- **磁盘满后无法自动恢复**

    - 检测到磁盘已满，所有日志降级输出到控制台

    - 需要手动重启程序恢复日志写入功能

- **队列容量必须为2的幂次方**，否则初始化抛出异常

---

## 🧪 单元测试

```bash
cd build
ctest --output-on-failure

```

核心测试用例：

- `test\_shutdown\_no\_data\_loss` — 正常关闭无日志丢失验证

- `test\_shutdown\_with\_backlog` — 验证丢弃、写入日志统计准确性

- `test\_rolling\_unique\_names` — 高频滚动无文件名冲突验证

- `test\_reopen\_size\_tracking` — 重启程序正确还原文件大小

---

## 📝 更新日志

### v1\.1\.0

#### 🐛 漏洞修复

- 修复一秒内多次滚动导致的文件名冲突（新增滚动序列号）

- 修复归档扫描函数误判活跃日志文件的问题

- 修复重启文件后无法还原文件大小的bug

- 修复关闭后初始化标记未重置，支持重复初始化

- 修复条件变量未唤醒，导致关闭卡顿的问题

---

## 📌 作者

维护人：Gabriel \| 面向C\+\+工程学习开源

适合学习高性能中间件、无锁队列、日志滚动底层原理。

> （注：文档部分内容可能由 AI 生成）
