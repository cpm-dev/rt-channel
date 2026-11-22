# rt-channel

A lock-free channel in shared memory between two Linux processes, carrying messages from a
10 ms periodic real-time task, with a health monitor that detects missed deadlines and a dead
producer. Measured under a noisy neighbor with no isolation, cgroup v2 limits, and a dedicated core.


## Result

![wakeup jitter by configuration](results/jitter.png)

| Configuration | p50 jitter | p99 jitter | p99.9 jitter | Max jitter | Lost cycles |
| --- | --- | --- | --- | --- | --- |
| Ring hot-path benchmark (local Mac) | 0.041 µs | 0.042 µs | 0.124 µs | 37.917 µs | 0 |
| No isolation | — | — | — | — | — |
| cgroup v2 limits | — | — | — | — | — |
| Dedicated core | — | — | — | — | — |

The local Mac value is a ring push/pop microbenchmark of 100,000 measured iterations. It measures
execution-duration variance, not 10 ms task wakeup jitter or shared-memory process latency. The remaining
isolation configurations have not been collected yet.

Hardware, kernel and settings: see `results/environment.txt`.

## Architecture

```
 producer (SCHED_FIFO, 10 ms)                     monitor
 ┌──────────────────────────┐   shared memory    ┌──────────────────────────────┐
 │ clock_nanosleep(ABSTIME) │   ┌────────────┐   │ drain ring                    │
 │ stamp Message            │──▶│ SPSC ring  │──▶│ latency + jitter histograms   │
 │ try_push (never blocks)  │   │ heartbeat  │   │ sequence gaps, overruns       │
 └──────────────────────────┘   └────────────┘   │ heartbeat timeout → dead      │
                                                 └──────────────────────────────┘
```


## Failure modes

| Failure | Detected by | Behavior |
| --- | --- | --- |
| Producer crashes | | |
| Monitor falls behind, ring fills | | |
| Producer overruns its period | | |
| Stale segment left after a crash | | |



## Next steps

- Run inside a Yocto image with a PREEMPT_RT kernel.
- A Go supervisor that launches workloads from a manifest with per-criticality cgroup limits.
- A Zephyr safety island on a Raspberry Pi Pico that watches the heartbeat over UART and triggers recovery.
- The same channel across two KVM or Xen guests over shared memory (the virtio idea).

## Build and run

```bash
cmake -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
cmake -B build-tsan -DSANITIZE=thread && cmake --build build-tsan -j && ctest --test-dir build-tsan
sudo scripts/run_experiment.sh 60 3
python3 scripts/plot_latency.py
```

## Pre-commit checks

Requires `clang-format` and `clang-tidy`:

```bash
scripts/pre_commit.sh format   # apply clang-format
scripts/pre_commit.sh check    # verify formatting without modifying files
scripts/pre_commit.sh lint     # build test targets and run clang-tidy
scripts/pre_commit.sh all      # formatting, build, clang-tidy, and tests
```

CI runs the same format and static-analysis checks on pull requests.
