#pragma once

#include <atomic>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <stdexcept>

namespace xlog {

template <typename T>
class MpscQueue {
public:
    explicit MpscQueue(size_t capacity) 
        : capacity_(capacity),
          mask_(capacity - 1),
          slots_(capacity),
          tail_(0),
          head_(0),
          dropped_(0)
    {
        // 容量必须是 2 的幂
        if ((capacity_ & mask_) != 0) {
            throw std::runtime_error("MpscQueue capacity must be power of 2");
        }

        for (size_t i = 0; i < capacity_; ++i) {
            slots_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    // 禁用拷贝
    MpscQueue(const MpscQueue&) = delete;
    MpscQueue& operator=(const MpscQueue&) = delete;

    // 允许移动
    MpscQueue(MpscQueue&&) = default;
    MpscQueue& operator=(MpscQueue&&) = default;

    // 多生产者安全 push
    bool push(T&& item) {
        // 原子地把 tail_ 加 1 **返回加 1 之前的旧值**
        uint64_t pos = tail_.fetch_add(1, std::memory_order_acq_rel);
        Slot& slot = slots_[pos & mask_];

        uint64_t expected_seq = pos;
        if (slot.sequence.load(std::memory_order_acquire) != expected_seq) {
            dropped_.fetch_add(1, std::memory_order_relaxed);
            return false;  // 队列满，丢弃
        }

        slot.data = std::move(item);
        slot.sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    // 单消费者 pop
    bool pop(T& item) {
        uint64_t pos = head_;
        Slot& slot = slots_[pos & mask_];

        if (slot.sequence.load(std::memory_order_acquire) != pos + 1) {
            return false; // 队列空或生产者未写完
        }

        item = std::move(slot.data);
        slot.sequence.store(pos + capacity_, std::memory_order_release); // 释放槽位
        ++head_;
        return true;
    }

    uint64_t dropped_count() const {
        return dropped_.load(std::memory_order_relaxed);
    }

private:
    struct Slot {
        std::atomic<uint64_t> sequence;
        T                     data;
    };

    std::vector<Slot> slots_;
    const uint64_t    mask_;
    const uint64_t    capacity_;

    alignas(64) std::atomic<uint64_t> tail_;    // 多生产者写指针
    alignas(64) uint64_t              head_;    // 单消费者读指针
    alignas(64) std::atomic<uint64_t> dropped_; // 丢弃计数
};

} // namespace xlog
