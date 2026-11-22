#include "rtchan/spsc_ring.hpp"

static rtchan::SpscRing<std::uint64_t, 1024> bench_ring;

void run_target_step() {
    static std::uint64_t value = 0;

    if (!bench_ring.try_push(value)) {
        return;
    }

    std::uint64_t out = 0;
    (void)bench_ring.try_pop(out);
    ++value;
}
