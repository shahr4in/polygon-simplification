#!/usr/bin/env python3
"""Run benchmark cases and record runtime and memory metrics as CSV."""

from __future__ import annotations

import argparse
import csv
import re
import statistics
import subprocess
import tempfile
import time
from pathlib import Path


TIME_PATTERN = re.compile(r"Elapsed \(wall clock\) time \(h:mm:ss or m:ss\): ([0-9:.]+)")
RSS_PATTERN = re.compile(r"Maximum resident set size \(kbytes\): (\d+)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--cases",
        default="benchmarks/cases.csv",
        help="Path to benchmark case manifest CSV.",
    )
    parser.add_argument(
        "--output",
        default="benchmarks/results.csv",
        help="Output CSV path for benchmark results.",
    )
    parser.add_argument(
        "--binary",
        default="./simplify",
        help="Path to the simplification executable.",
    )
    parser.add_argument(
        "--repeats",
        type=int,
        default=5,
        help="Number of timing repeats per case.",
    )
    parser.add_argument(
        "--warmup",
        type=int,
        default=1,
        help="Number of warmup runs per case before recording metrics.",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=float,
        default=0.0,
        help="Per-run timeout in seconds. Use 0 to disable timeouts.",
    )
    return parser.parse_args()


def parse_elapsed_seconds(raw: str) -> float:
    parts = raw.strip().split(":")
    if len(parts) == 2:
        minutes = int(parts[0])
        seconds = float(parts[1])
        return minutes * 60.0 + seconds
    if len(parts) == 3:
        hours = int(parts[0])
        minutes = int(parts[1])
        seconds = float(parts[2])
        return hours * 3600.0 + minutes * 60.0 + seconds
    raise ValueError(f"Unexpected elapsed time format: {raw!r}")


def load_cases(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def count_input_metrics(path: Path) -> tuple[int, int]:
    vertices = 0
    rings: set[int] = set()
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            vertices += 1
            rings.add(int(row["ring_id"]))
    return vertices, len(rings)


def count_output_vertices(stdout_text: str) -> int:
    count = 0
    for line in stdout_text.splitlines():
        if line.startswith("Total "):
            break
        if not line or line == "ring_id,vertex_id,x,y":
            continue
        count += 1
    return count


def stage_input(case_name: str, source_path: Path, temp_dir: Path) -> Path:
    safe_name = f"{case_name}.csv"
    staged_path = temp_dir / safe_name
    staged_path.write_bytes(source_path.read_bytes())
    return staged_path


def run_once(binary: Path, input_path: Path, target_vertices: str, timeout_seconds: float) -> tuple[float, int, str]:
    command = [
        "/usr/bin/time",
        "-v",
        str(binary),
        str(input_path),
        str(target_vertices),
    ]
    timeout = timeout_seconds if timeout_seconds > 0 else None
    start = time.perf_counter()
    result = subprocess.run(command, capture_output=True, text=True, check=True, timeout=timeout)
    elapsed_ms = (time.perf_counter() - start) * 1000.0

    elapsed_match = TIME_PATTERN.search(result.stderr)
    rss_match = RSS_PATTERN.search(result.stderr)
    if elapsed_match is None or rss_match is None:
        raise RuntimeError(f"Failed to parse /usr/bin/time -v output:\n{result.stderr}")

    max_rss_kb = int(rss_match.group(1))
    return elapsed_ms, max_rss_kb, result.stdout


def main() -> int:
    args = parse_args()
    repo_root = Path.cwd()
    binary = (repo_root / args.binary).resolve()
    cases_path = (repo_root / args.cases).resolve()
    output_path = (repo_root / args.output).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)

    rows: list[dict[str, object]] = []
    cases = load_cases(cases_path)

    with tempfile.TemporaryDirectory(prefix="polygon-bench-") as temp_name:
        temp_dir = Path(temp_name)
        total_cases = len(cases)
        for index, case in enumerate(cases, start=1):
            case_name = case["case"]
            source_path = (repo_root / case["input_path"]).resolve()
            target_vertices = case["target_vertices"]
            staged_input = stage_input(case_name, source_path, temp_dir)
            input_vertices, rings = count_input_metrics(source_path)
            print(
                f"[{index}/{total_cases}] {case_name}: "
                f"input_vertices={input_vertices}, rings={rings}, target={target_vertices}",
                flush=True,
            )

            try:
                for _ in range(args.warmup):
                    run_once(binary, staged_input, target_vertices, args.timeout_seconds)

                runtimes_ms: list[float] = []
                rss_values_kb: list[int] = []
                output_vertices = None
                for _ in range(args.repeats):
                    runtime_ms, max_rss_kb, stdout_text = run_once(
                        binary, staged_input, target_vertices, args.timeout_seconds
                    )
                    runtimes_ms.append(runtime_ms)
                    rss_values_kb.append(max_rss_kb)
                    if output_vertices is None:
                        output_vertices = count_output_vertices(stdout_text)
            except subprocess.TimeoutExpired:
                rows.append(
                    {
                        "case": case_name,
                        "input_path": case["input_path"],
                        "target_vertices": int(target_vertices),
                        "input_vertices": input_vertices,
                        "rings": rings,
                        "output_vertices": "",
                        "runtime_mean_ms": "",
                        "runtime_stdev_ms": "",
                        "runtime_min_ms": "",
                        "runtime_max_ms": "",
                        "max_rss_mean_kb": "",
                        "max_rss_max_kb": "",
                        "repeats": args.repeats,
                        "status": f"timeout>{args.timeout_seconds}s",
                    }
                )
                print(f"  skipped: timeout after {args.timeout_seconds}s", flush=True)
                continue

            rows.append(
                {
                    "case": case_name,
                    "input_path": case["input_path"],
                    "target_vertices": int(target_vertices),
                    "input_vertices": input_vertices,
                    "rings": rings,
                    "output_vertices": output_vertices if output_vertices is not None else 0,
                    "runtime_mean_ms": round(statistics.mean(runtimes_ms), 6),
                    "runtime_stdev_ms": round(statistics.pstdev(runtimes_ms), 6),
                    "runtime_min_ms": round(min(runtimes_ms), 6),
                    "runtime_max_ms": round(max(runtimes_ms), 6),
                    "max_rss_mean_kb": round(statistics.mean(rss_values_kb), 6),
                    "max_rss_max_kb": max(rss_values_kb),
                    "repeats": args.repeats,
                    "status": "ok",
                }
            )
            print(
                f"  runtime_mean_ms={rows[-1]['runtime_mean_ms']}, "
                f"max_rss_mean_kb={rows[-1]['max_rss_mean_kb']}",
                flush=True,
            )

    with output_path.open("w", newline="", encoding="utf-8") as handle:
        fieldnames = [
            "case",
            "input_path",
            "target_vertices",
            "input_vertices",
            "rings",
            "output_vertices",
            "runtime_mean_ms",
            "runtime_stdev_ms",
            "runtime_min_ms",
            "runtime_max_ms",
            "max_rss_mean_kb",
            "max_rss_max_kb",
            "repeats",
            "status",
        ]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote benchmark results to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
