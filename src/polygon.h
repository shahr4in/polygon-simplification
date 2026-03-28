#ifndef POLYGON_SIMPLIFICATION_POLYGON_H
#define POLYGON_SIMPLIFICATION_POLYGON_H

#include "geometry.h"

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace polygon_simplification {

/**
 * @brief One vertex slot in a ring's cyclic linked structure.
 */
struct Node {
    /** @brief Stored vertex coordinates. */
    Vec2 point;
    /** @brief Index of the previous alive vertex in the ring. */
    int prev = -1;
    /** @brief Index of the next alive vertex in the ring. */
    int next = -1;
    /** @brief Whether this slot currently participates in the ring. */
    bool alive = true;
    /** @brief Version counter used to invalidate stale simplification candidates. */
    std::uint64_t version = 0;
};

/**
 * @brief Mutable state for one polygon ring.
 */
struct RingState {
    /** @brief Logical ring id from the input. */
    int ring_id = -1;
    /** @brief Stable storage for original and generated nodes. */
    std::vector<Node> nodes;
    /** @brief Index of any currently alive node for traversal start. */
    int any_alive = -1;
    /** @brief Number of alive vertices in the ring. */
    int alive_count = 0;
    /** @brief Monotonic generation counter for accepted edits. */
    std::uint64_t generation = 0;
    /** @brief Signed area of the original input ring. */
    double original_signed_area = 0.0;
    /** @brief Minimum x-coordinate of the active ring bounding box. */
    double min_x = 0.0;
    /** @brief Maximum x-coordinate of the active ring bounding box. */
    double max_x = 0.0;
    /** @brief Minimum y-coordinate of the active ring bounding box. */
    double min_y = 0.0;
    /** @brief Maximum y-coordinate of the active ring bounding box. */
    double max_y = 0.0;
    /** @brief Cell size used by the uniform-grid edge index. */
    double grid_cell_size = 1.0;
    /** @brief Mapping from grid cell key to active edge start indices. */
    std::unordered_map<long long, std::vector<int>> edge_grid;
    /** @brief Reverse mapping from edge start index to occupied grid cells. */
    std::vector<std::vector<long long>> edge_grid_cells;
    /** @brief Whether the current edge grid matches the active ring state. */
    bool edge_grid_ready = false;
};

/**
 * @brief One area-preserving collapse candidate.
 */
struct Candidate {
    /** @brief Ring to which the candidate belongs. */
    int ring_id = -1;
    /** @brief Index of vertex B in the local chain A-B-C-D. */
    int b = -1;
    /** @brief Index of vertex C in the local chain A-B-C-D. */
    int c = -1;
    /** @brief Index of vertex A in the local chain A-B-C-D. */
    int a = -1;
    /** @brief Index of vertex D in the local chain A-B-C-D. */
    int d = -1;
    /** @brief Replacement point E used to collapse A-B-C-D into A-E-D. */
    Vec2 e;
    /** @brief Local areal displacement score used for greedy ordering. */
    double displacement = 0.0;
    /** @brief Node versions captured when the candidate was generated. */
    std::array<std::uint64_t, 4> versions{};
};

/**
 * @brief Strict weak ordering for the candidate priority queue.
 */
struct CandidateCompare {
    /**
     * @brief Orders candidates by increasing displacement, then by stable tiebreakers.
     * @param lhs Left candidate.
     * @param rhs Right candidate.
     * @return `true` when @p lhs should appear after @p rhs in the priority queue.
     */
    bool operator()(const Candidate& lhs, const Candidate& rhs) const;
};

/**
 * @brief Top-level polygon container consisting of one exterior ring and optional holes.
 */
struct PolygonData {
    /** @brief Rings in input order, where ring `0` is the exterior ring. */
    std::vector<RingState> rings;
};

/**
 * @brief Extracts the current alive vertices of a ring in traversal order.
 * @param ring Ring state to traverse.
 * @return Ordered alive points for the ring.
 * @throws std::runtime_error If the ring linkage is corrupt.
 */
std::vector<Vec2> alive_ring_points(const RingState& ring);

/**
 * @brief Computes the total signed area across all rings.
 * @param rings Polygon rings.
 * @return Sum of signed ring areas.
 */
double total_signed_area(const std::vector<RingState>& rings);

/**
 * @brief Counts alive vertices across all rings.
 * @param rings Polygon rings.
 * @return Total number of currently active vertices.
 */
int total_vertex_count(const std::vector<RingState>& rings);

/**
 * @brief Validates geometric and topological constraints for the polygon.
 * @param rings Polygon rings to validate.
 * @return `true` if the polygon is non-degenerate, non-self-intersecting, and hole-consistent.
 */
bool polygon_is_valid(const std::vector<RingState>& rings);

/**
 * @brief Tests whether applying a local collapse candidate would preserve polygon validity.
 * @param rings Current polygon rings before the collapse.
 * @param ring_id Ring being modified.
 * @param a Index of vertex A in the local chain A-B-C-D.
 * @param d Index of vertex D in the local chain A-B-C-D.
 * @param e Proposed replacement point E.
 * @return `true` if replacing A-B-C-D with A-E-D preserves local and global validity.
 */
bool collapse_preserves_validity(std::vector<RingState>& rings, int ring_id, int a, int d, const Vec2& e);

/**
 * @brief Builds the uniform-grid spatial index for a ring if it is stale.
 * @param ring Ring whose active edges should be indexed.
 */
void ensure_edge_grid(RingState& ring);

/**
 * @brief Updates the grid entries touched by one accepted collapse.
 * @param ring Updated ring after the collapse has been committed.
 * @param a Index of vertex A in the collapsed local chain.
 * @param b Index of removed vertex B.
 * @param c Index of removed vertex C.
 * @param e Index of the newly inserted replacement vertex E.
 */
void update_edge_grid_after_collapse(RingState& ring, int a, int b, int c, int e);

}  // namespace polygon_simplification

#endif
