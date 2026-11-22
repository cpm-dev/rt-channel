#include <chrono>
#include <cstdint>
#include <fstream>
#include <vector>
#include "rtchan/spsc_ring.hpp"

extern void run_target_step(); // Your compiled C++ code under test


int main() {
    constexpr std::size_t warmup_iters = 1'000;
    constexpr std::size_t measured_iters = 100'000;

    std::vector<int64_t> durations_ns(measured_iters);

    // Warmup caches & branch predictors
    for (std::size_t i = 0; i < warmup_iters; ++i) {
        run_target_step();
    }

    // Hot path measurement (no I/O, no allocations)
    for (std::size_t i = 0; i < measured_iters; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        run_target_step();
        const auto t1 = std::chrono::steady_clock::now();
        durations_ns[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    }

    // Dump raw int64 array to disk for Python analysis
    std::ofstream out("durations.bin", std::ios::binary);
    const std::streamsize byte_count = static_cast<std::streamsize>(
            durations_ns.size() * sizeof(std::int64_t));
    out.write(reinterpret_cast<const char*>(durations_ns.data()), byte_count);
    return 0;
}

