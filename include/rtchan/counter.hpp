#pragma once

#include <cstdint>

namespace rtchan {

inline constexpr std::uint8_t kCounterMask = 0x0F;

// Advances a 4-bit rolling counter, wrapping from 15 to 0.
constexpr std::uint8_t next_counter(std::uint8_t current) noexcept {
    return static_cast<std::uint8_t>(((current & kCounterMask) + 1u) & kCounterMask);
}

// Reports whether current is exactly the next valid 4-bit counter value.
constexpr bool counter_continuous(std::uint8_t previous,
                                 std::uint8_t current) noexcept {
    if (previous > kCounterMask || current > kCounterMask) {
        return false;
    }
    return next_counter(previous) == current;
}

}  // namespace rtchan
