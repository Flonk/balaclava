#pragma once

#include <vector>
#include <atomic>
#include <cstring>
#include <algorithm>

namespace balaclava {

struct SpscRing {
    alignas(64) std::vector<float> buf;
    alignas(64) std::atomic<size_t> head{0}; // producer
    alignas(64) std::atomic<size_t> tail{0}; // consumer
    size_t mask; // size must be power-of-two

    explicit SpscRing(size_t pow2_capacity) {
        size_t cap = 1;
        while (cap < pow2_capacity) cap <<= 1;
        buf.resize(cap);
        mask = cap - 1;
    }

    // producer write (audio thread)
    size_t write(const float* src, size_t n) {
        size_t h = head.load(std::memory_order_relaxed);
        size_t t = tail.load(std::memory_order_acquire);
        size_t free = (buf.size() - (h - t));
        n = std::min(n, free);
        size_t pos = h & mask;
        size_t first = std::min(n, buf.size() - pos);
        std::memcpy(&buf[pos], src, first * sizeof(float));
        if (first < n) {
            std::memcpy(&buf[0], src + first, (n - first) * sizeof(float));
        }
        head.store(h + n, std::memory_order_release);
        return n;
    }

    // consumer read (UI/processing thread)
    size_t read(float* dst, size_t n) {
        size_t h = head.load(std::memory_order_acquire);
        size_t t = tail.load(std::memory_order_relaxed);
        size_t avail = h - t;
        n = std::min(n, avail);
        size_t pos = t & mask;
        size_t first = std::min(n, buf.size() - pos);
        std::memcpy(dst, &buf[pos], first * sizeof(float));
        if (first < n) {
            std::memcpy(dst + first, &buf[0], (n - first) * sizeof(float));
        }
        tail.store(t + n, std::memory_order_release);
        return n;
    }

    // consumer skip (advance tail without copying)
    size_t skip(size_t n) {
        size_t h = head.load(std::memory_order_acquire);
        size_t t = tail.load(std::memory_order_relaxed);
        size_t avail = h - t;
        n = std::min(n, avail);
        tail.store(t + n, std::memory_order_release);
        return n;
    }

    size_t available() const {
        return head.load(std::memory_order_acquire) - tail.load(std::memory_order_acquire);
    }
};

} // namespace balaclava
