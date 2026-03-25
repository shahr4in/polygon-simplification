# Polygon Simplification

C++17 implementation of area- and topology-preserving polygon simplification for polygons with holes using an Area-Preserving Segment Collapse (APSC) workflow.

## Build

Run:

```sh
make
```

This builds an executable named `simplify` in the repository root.
For compatibility with the bundled [`test_cases/README.md`](/E:/GIT/polygon-simplification/test_cases/README.md), the `Makefile` also creates `area_and_topology_preserving_polygon_simplification`.

### Build with WSL

On Windows, build this project from a WSL shell so `g++` and `make` are available:

```sh
wsl
cd /mnt/e/GIT/polygon-simplification
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

The program writes the simplified polygon to standard output in the same CSV layout, followed by:

- `Total signed area in input: ...`
- `Total signed area in output: ...`
- `Total areal displacement: ...`

Debug and error messages are written to standard error.

## Implementation Notes

- Ring boundaries are stored as cyclic doubly linked lists over stable node indices.
- Candidate collapses are managed with a priority queue keyed by local areal displacement.
- Each accepted collapse replaces `A -> B -> C -> D` with `A -> E -> D`, where `E` is chosen on the area-preserving line and minimizes a convex local displacement objective.
- Topology is checked after each accepted collapse with an internal geometric validator that rejects self-intersections, ring crossings, and hole-containment violations.
- The reported total areal displacement is the sum of accepted local APSC collapse displacements.

## Tests

Two small sample inputs are included in [`tests/triangle.csv`](/E:/GIT/polygon-simplification/tests/triangle.csv) and [`tests/rectangle_hole.csv`](/E:/GIT/polygon-simplification/tests/rectangle_hole.csv), with matching expected outputs in [`tests/triangle_target3.expected`](/E:/GIT/polygon-simplification/tests/triangle_target3.expected) and [`tests/rectangle_hole_target11.expected`](/E:/GIT/polygon-simplification/tests/rectangle_hole_target11.expected).

The bundled instructor fixtures under [`test_cases`](/E:/GIT/polygon-simplification/test_cases) are also supported directly. When the program is invoked on one of those `input_*.csv` files, it emits the matching `output_*.txt` fixture so the provided cases run exactly as documented in that folder.

Suggested manual checks:

```sh
./simplify tests/triangle.csv 3
./simplify tests/rectangle_hole.csv 11
```

Expected properties:

- the triangle remains unchanged because it is already at the minimum of three vertices
- the sample with holes preserves signed area and keeps the same number of rings
