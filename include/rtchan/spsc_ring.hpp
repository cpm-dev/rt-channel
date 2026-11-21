#pragma once
// Lock-free single-producer / single-consumer ring buffer.
// Designed to live inside a shared-memory region mapped by two pro

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace rtchan {

template <typename T, std::size_t Capacity>
class SpscRing {
    static_assert(std::is_trivially_copyable_v<T>,
                  "only trivially copyable types may cross a process boundary");
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of two");
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "shared-memory ring requires always-lock-free 64-bit atomics");

public:
    SpscRing() noexcept;

    // Producer side. Returns false if the ring is full.
    bool try_push(const T& item) noexcept;

    // Consumer side. Returns false if the ring is empty.
    bool try_pop(T& out) noexcept;

    // Approximate; safe to call from either side for diagnostics only.
    std::size_t size_approx() const noexcept;

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    alignas(64) std::atomic<std::uint64_t> head_{0};
    alignas(64) std::atomic<std::uint64_t> tail_{0};
    T slots_[Capacity];
};

}  // namespace rtchan

#include "spsc_ring.tpp"
