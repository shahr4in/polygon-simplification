# Polygon Simplification

C++17 implementation of area and topology-preserving polygon simplification for polygons with holes using an Area-Preserving Segment Collapse (APSC) workflow.

Source files are located under [`src`](src).

## Build

Run:

```sh
make
```

This builds an executable named `simplify` in the repository root.
The `Makefile` also creates `area_and_topology_preserving_polygon_simplification` for naming compatibility with the bundled [`test_cases/README.md`](test_cases/README.md).

### Build with WSL

On Windows, build this project from a WSL shell so `g++` and `make` are available:

```sh
wsl
cd /path/to/polygon-simplification
make
```

If needed, install the basic toolchain inside WSL first:

```sh
sudo apt update
sudo apt install build-essential
```

## Usage

```sh
./simplify <input_file.csv> <target_vertices>
```

The compatibility executable accepts the same arguments:

```sh
./area_and_topology_preserving_polygon_simplification <input_file.csv> <target_vertices>
```

Input CSV format:

```text
ring_id,vertex_id,x,y
0,0,...
0,1,...
...
```

Requirements assumed by the program:

- ring `0` is the exterior ring and is counterclockwise
- interior rings are clockwise
- ring ids are contiguous and grouped
- vertex ids within each ring start at `0` and are contiguous
- the input polygon is topologically valid

The implementation trusts this input-validity precondition during CSV loading and enforces topology preservation during accepted simplification steps.

The program writes the simplified polygon to standard output in the same CSV layout, followed by:

- `Total signed area in input: ...`
- `Total signed area in output: ...`
- `Total areal displacement: ...`

Debug and error messages are written to standard error.

## Implementation Notes

- Ring boundaries are stored as cyclic doubly linked lists over stable node indices.
- Candidate collapses are managed with a priority queue keyed by local areal displacement.
- Active edges are indexed in a uniform-grid spatial index so local validity checks query nearby segments instead of scanning whole rings.
- Each accepted collapse replaces `A -> B -> C -> D` with `A -> E -> D`, where `E` is chosen on the area-preserving line and minimizes a convex local displacement objective.
- Topology is checked after each accepted collapse with an internal geometric validator that rejects self-intersections, ring crossings and hole-containment violations.
- The reported total areal displacement is the sum of accepted local APSC collapse displacements.

## Tests

The repository includes several standalone sample cases under [`tests`](tests):

- [`tests/triangle.csv`](tests/triangle.csv) with expected output [`tests/triangle_target3.expected`](tests/triangle_target3.expected)
- [`tests/rectangle_hole.csv`](tests/rectangle_hole.csv) with expected output [`tests/rectangle_hole_target11.expected`](tests/rectangle_hole_target11.expected)
- [`tests/rectangle_two_holes.csv`](tests/rectangle_two_holes.csv) with expected output [`tests/rectangle_two_holes_target7.expected`](tests/rectangle_two_holes_target7.expected)
- [`tests/cushion_hexagonal_hole.csv`](tests/cushion_hexagonal_hole.csv) with expected output [`tests/cushion_hexagonal_hole_target13.expected`](tests/cushion_hexagonal_hole_target13.expected)
- [`tests/blob_two_holes.csv`](tests/blob_two_holes.csv) with expected output [`tests/blob_two_holes_target17.expected`](tests/blob_two_holes_target17.expected)
- [`tests/wavy_three_holes.csv`](tests/wavy_three_holes.csv) with expected output [`tests/wavy_three_holes_target21.expected`](tests/wavy_three_holes_target21.expected)
- [`tests/lake_two_islands.csv`](tests/lake_two_islands.csv) with expected output [`tests/lake_two_islands_target17.expected`](tests/lake_two_islands_target17.expected)

The standalone `tests` cases provide lightweight regression coverage for local development. The bundled instructor fixtures under [`test_cases`](test_cases) can also be executed directly with the same program interface.

Example local checks:

```sh
./simplify tests/triangle.csv 3
./simplify tests/rectangle_hole.csv 11
./simplify tests/rectangle_two_holes.csv 7
./simplify tests/cushion_hexagonal_hole.csv 13
./area_and_topology_preserving_polygon_simplification test_cases/input_rectangle_with_two_holes.csv 7
```

### Test Results

The standalone cases under [`tests`](tests) are intended to be checked by diffing program output against the corresponding `*.expected` file. A typical check from WSL is:

```sh
diff -u tests/triangle_target3.expected <(./simplify tests/triangle.csv 3)
diff -u tests/rectangle_hole_target11.expected <(./simplify tests/rectangle_hole.csv 11)
```

Recent local runs confirmed successful execution on the following representative inputs:

| Case | Input vertices | Rings | Output vertices | Status |
|---|---:|---:|---:|---|
| `triangle` | 3 | 1 | 3 | `ok` |
| `rectangle_hole` | 12 | 3 | 11 | `ok` |
| `rectangle_two_holes` | 12 | 3 | 9 | `ok` |
| `cushion_hexagonal_hole` | 22 | 2 | 13 | `ok` |
| `blob_two_holes` | 36 | 3 | 17 | `ok` |
| `wavy_three_holes` | 43 | 4 | 21 | `ok` |
| `lake_two_islands` | 81 | 3 | 17 | `ok` |
| `original_01` | 1860 | 1 | 99 | `ok` |
| `original_02` | 8605 | 1 | 99 | `ok` |
| `original_06` | 14122 | 1 | 99 | `ok` |
| `original_09` | 409998 | 1 | 99 | `ok` |

The instructor `test_cases` inputs now run through the same simplification code path as every other input file, so they can be used as genuine correctness and performance tests.

### Correctness Notes

The implementation checks correctness in two ways during simplification:

- area preservation is enforced locally by constructing each replacement point on the area-preserving line, and the program reports total signed area for both input and output polygons
- topology preservation is enforced after every accepted collapse by rejecting self-intersections, inter-ring crossings, degenerate edges, and invalid hole containment

For the standalone fixtures in [`tests`](tests), the expected outputs preserve signed area to the printed floating-point precision and keep the ring count unchanged.

### Areal Displacement Summary

The program reports total areal displacement for each simplification run. On the current standalone fixtures, the reported values are:

| Case | Input vertices | Output vertices | Vertices removed | Total areal displacement | Displacement per removed vertex |
|---|---:|---:|---:|---:|---:|
| `triangle` | 3 | 3 | 0 | 0.000000e+00 | n/a |
| `rectangle_hole` | 12 | 11 | 1 | 1.600000e+00 | 1.6000 |
| `rectangle_two_holes` | 12 | 11 | 1 | 1.600000e+00 | 1.6000 |
| `cushion_hexagonal_hole` | 22 | 13 | 9 | 3.840642e+02 | 42.6738 |
| `blob_two_holes` | 36 | 17 | 19 | 7.433220e+04 | 3912.2211 |
| `wavy_three_holes` | 43 | 21 | 22 | 1.254156e+05 | 5700.7091 |
| `lake_two_islands` | 81 | 17 | 64 | 1.004398e+05 | 1569.3719 |

These values are most useful for comparing runs of the same implementation. They should not be over-interpreted across very different shapes because the raw displacement scale depends on the coordinate scale of the input polygon.

This implementation uses the baseline greedy APSC workflow. It does not currently include non-greedy ordering, look-ahead selection, or a post-processing pass to further reduce areal displacement or Hausdorff distance.

## Benchmarks

The repository includes a benchmark workflow under [`benchmarks`](benchmarks) for collecting runtime and memory metrics as a function of input size.

Benchmark cases are listed in [`benchmarks/cases.csv`](benchmarks/cases.csv).

From WSL:

```sh
wsl
cd /path/to/polygon-simplification
make
python3 benchmarks/run_benchmarks.py --cases benchmarks/cases_fast.csv --repeats 1 --warmup 0 --timeout-seconds 30
python3 benchmarks/plot_results.py
```

This writes:

- `benchmarks/results.csv` with per-case input size, ring count, output size, runtime and peak memory metrics
- `benchmarks/plots/runtime_vs_input_vertices.svg`
- `benchmarks/plots/memory_vs_input_vertices.svg`

The runner uses Python's high-resolution process timing for runtime and `/usr/bin/time -v` for:

- elapsed wall-clock time
- maximum resident set size in kilobytes

The most relevant CSV columns for an experimental evaluation are:

- `input_vertices`
- `rings`
- `target_vertices`
- `output_vertices`
- `runtime_mean_ms`
- `runtime_stdev_ms`
- `max_rss_mean_kb`
- `max_rss_max_kb`

The repository provides three benchmark manifests:

- [`benchmarks/cases_fast.csv`](benchmarks/cases_fast.csv) for the quickest usable run
- [`benchmarks/cases_quick.csv`](benchmarks/cases_quick.csv) for a short run while drafting your report
- [`benchmarks/cases.csv`](benchmarks/cases.csv) for a broader sweep including larger lake datasets

If you want an even shorter benchmark run while drafting the report, reduce `--repeats`. For example:

```sh
python3 benchmarks/run_benchmarks.py --cases benchmarks/cases_fast.csv --repeats 1 --warmup 0 --timeout-seconds 30
```

### Experimental Evaluation

The most recent local benchmark summary from `benchmarks/results.csv` is:

| Case | Input vertices | Rings | Mean runtime (ms) | Mean max RSS (KB) | Status |
|---|---:|---:|---:|---:|---|
| `triangle` | 3 | 1 | 8.127 | 3968 | `ok` |
| `rectangle_hole` | 12 | 3 | 5.233 | 3968 | `ok` |
| `rectangle_two_holes` | 12 | 3 | 4.915 | 3968 | `ok` |
| `cushion_hexagonal_hole` | 22 | 2 | 4.912 | 4096 | `ok` |
| `blob_two_holes` | 36 | 3 | 4.497 | 3968 | `ok` |
| `wavy_three_holes` | 43 | 4 | 4.814 | 4096 | `ok` |
| `lake_two_islands` | 81 | 3 | 5.405 | 4224 | `ok` |
| `original_01` | 1860 | 1 | 12.071 | 4480 | `ok` |
| `original_02` | 8605 | 1 | 48.422 | 6828 | `ok` |
| `original_06` | 14122 | 1 | 84.087 | 8968 | `ok` |
| `original_09` | 409998 | 1 | 6471.089 | 167672 | `ok` |

In this run, memory usage grew from roughly 4 MB on the smallest cases to about 167 MB on the 409,998-vertex `original_09` dataset, while runtime ranged from about 4.5-8.1 ms on the smallest cases to about 6.47 s on `original_09`. These results show that the implementation can process an instructor dataset well above the 100,000-vertex threshold within practical runtime on the test machine.
