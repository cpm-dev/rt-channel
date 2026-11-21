#pragma once

namespace rtchan {

template <typename T, std::size_t Capacity>
SpscRing<T, Capacity>::SpscRing() noexcept = default;

template <typename T, std::size_t Capacity>
bool SpscRing<T, Capacity>::try_push(const T& item) noexcept {
    const std::uint64_t tail = tail_.load(std::memory_order_relaxed);
    const std::uint64_t head = head_.load(std::memory_order_acquire);

    if (tail - head == Capacity) {
        return false;
    }

    slots_[tail & (Capacity - 1)] = item;
    tail_.store(tail + 1, std::memory_order_release);
    return true;
}

template <typename T, std::size_t Capacity>
bool SpscRing<T, Capacity>::try_pop(T& out) noexcept {
    const std::uint64_t head = head_.load(std::memory_order_relaxed);
    const std::uint64_t tail = tail_.load(std::memory_order_acquire);

    if (tail == head) {
        return false;
    }

    out = slots_[head & (Capacity - 1)];
    head_.store(head + 1, std::memory_order_release);
    return true;
}

template <typename T, std::size_t Capacity>
std::size_t SpscRing<T, Capacity>::size_approx() const noexcept {
    const std::uint64_t tail = tail_.load(std::memory_order_relaxed);
    const std::uint64_t head = head_.load(std::memory_order_relaxed);
    const std::uint64_t used = tail - head;

    if (used > Capacity) {
        return 0;
    }
    return static_cast<std::size_t>(used);
}

} 
