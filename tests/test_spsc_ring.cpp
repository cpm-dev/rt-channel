// Behavior the ring buffer must satisfy. No external framework, so CI needs no downloads.
// Run normally, then under ThreadSanitizer:  cmake -B build-tsan -DSANITIZE=thread

#include <cstdint>
#include <cstdio>
#include <thread>

#include "rtchan/message.hpp"
#include "rtchan/spsc_ring.hpp"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

void empty_ring_pop_fails() {
    rtchan::SpscRing<int, 4> r;
    int out = -1;
    CHECK(!r.try_pop(out));
    CHECK(out == -1);  // must not write on failure
    CHECK(r.size_approx() == 0);
}

void push_until_full() {
    rtchan::SpscRing<int, 4> r;
    for (int i = 0; i < 4; ++i) CHECK(r.try_push(i));
    CHECK(!r.try_push(99));  // full at exactly Capacity items
    CHECK(r.size_approx() == 4);
}

void fifo_order() {
    rtchan::SpscRing<int, 8> r;
    for (int i = 0; i < 5; ++i) CHECK(r.try_push(i));
    for (int i = 0; i < 5; ++i) {
        int out = -1;
        CHECK(r.try_pop(out));
        CHECK(out == i);
    }
}

void wraparound_many_times() {
    rtchan::SpscRing<int, 4> r;
    for (int i = 0; i < 1000; ++i) {
        CHECK(r.try_push(i));
        CHECK(r.try_push(i + 1));
        int a = -1, b = -1;
        CHECK(r.try_pop(a));
        CHECK(r.try_pop(b));
        CHECK(a == i && b == i + 1);
    }
    CHECK(r.size_approx() == 0);
}

void full_then_drain_then_reuse() {
    rtchan::SpscRing<int, 4> r;
    for (int i = 0; i < 4; ++i) CHECK(r.try_push(i));
    int out;
    for (int i = 0; i < 4; ++i) CHECK(r.try_pop(out));
    CHECK(!r.try_pop(out));
    CHECK(r.try_push(42));
    CHECK(r.try_pop(out) && out == 42);
}

void message_round_trip() {
    rtchan::SpscRing<rtchan::Message, 16> r;
    rtchan::Message m{7, 100, 90, 95, rtchan::kFlagOverrun, 0};
    CHECK(r.try_push(m));
    rtchan::Message out{};
    CHECK(r.try_pop(out));
    CHECK(out.seq == 7 && out.t_send_ns == 100 && out.flags == rtchan::kFlagOverrun);
}

// The real test: one producer thread, one consumer thread, strict ordering, no loss.
// Under ThreadSanitizer this catches missing acquire/release.
void concurrent_ordering() {
    static rtchan::SpscRing<std::uint64_t, 1024> r;
    constexpr std::uint64_t kN = 2'000'000;
    std::thread producer([] {
        for (std::uint64_t i = 0; i < kN;) {
            if (r.try_push(i)) ++i;
        }
    });
    std::uint64_t expected = 0;
    bool in_order = true;
    while (expected < kN) {
        std::uint64_t v;
        if (r.try_pop(v)) {
            if (v != expected) in_order = false;
            ++expected;
        }
    }
    producer.join();
    CHECK(in_order);
    CHECK(r.size_approx() == 0);
}

}  // namespace

int main() {
    empty_ring_pop_fails();
    push_until_full();
    fifo_order();
    wraparound_many_times();
    full_then_drain_then_reuse();
    message_round_trip();
    if (g_failures == 0) {
        concurrent_ordering();  // would spin forever on the stub, so only run once the basics pass
    } else {
        std::fprintf(stderr, "skipping concurrent_ordering: basic tests failing\n");
    }
    if (g_failures) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    std::puts("all tests passed");
    return 0;
}
