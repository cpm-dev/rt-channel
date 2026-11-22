import subprocess
from pathlib import Path

import numpy as np

P99_JITTER_BUDGET_NS = 5_000  # e.g., 5 microseconds


def test_cpp_p99_jitter(tmp_path):
    project_root = Path(__file__).resolve().parent.parent
    bench_jitter = project_root / "build" / "bench_jitter"

    # Run with tmp_path as cwd so durations.bin is written to the test workspace.
    subprocess.run([bench_jitter], cwd=tmp_path, check=True)

    durations_ns = np.fromfile(tmp_path / "durations.bin", dtype=np.int64)
    assert durations_ns.size > 0

    jitter_ns = np.abs(np.diff(durations_ns))
    p99_jitter_ns = np.percentile(jitter_ns, 99)

    assert p99_jitter_ns <= P99_JITTER_BUDGET_NS, (
        f"P99 jitter {p99_jitter_ns / 1e3:.2f} us exceeded budget "
        f"{P99_JITTER_BUDGET_NS / 1e3:.2f} us"
    )

