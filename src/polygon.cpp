#include "polygon.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace polygon_simplification {

namespace {

struct EdgeRef {
    int ring_id;
    int start_idx;
    int end_idx;
};

long long cell_key(int cell_x, int cell_y) {
    return (static_cast<long long>(cell_x) << 32) ^
           static_cast<unsigned int>(cell_y);
}

bool bounding_boxes_overlap(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d) {
    const double min_ax = std::min(a.x, b.x);
    const double max_ax = std::max(a.x, b.x);
    const double min_ay = std::min(a.y, b.y);
    const double max_ay = std::max(a.y, b.y);
    const double min_cx = std::min(c.x, d.x);
    const double max_cx = std::max(c.x, d.x);
    const double min_cy = std::min(c.y, d.y);
    const double max_cy = std::max(c.y, d.y);
    return max_ax + kEps >= min_cx && max_cx + kEps >= min_ax &&
           max_ay + kEps >= min_cy && max_cy + kEps >= min_ay;
}

bool edge_has_endpoint(const EdgeRef& edge, int idx) {
    return edge.start_idx == idx || edge.end_idx == idx;
}

bool same_ring_edge_adjacent_to_new_edge(const EdgeRef& edge, int a, int b, int c, int d) {
    return edge_has_endpoint(edge, a) || edge_has_endpoint(edge, b) ||
           edge_has_endpoint(edge, c) || edge_has_endpoint(edge, d);
}

std::vector<EdgeRef> alive_edges(const RingState& ring) {
    std::vector<EdgeRef> edges;
    if (ring.alive_count == 0 || ring.any_alive < 0) {
        return edges;
    }

    edges.reserve(static_cast<std::size_t>(ring.alive_count));
    int start = ring.any_alive;
    int current = start;
    do {
        const int next = ring.nodes[current].next;
        edges.push_back({ring.ring_id, current, next});
        current = next;
    } while (current != start);
    return edges;
}

bool points_are_distinct(const Vec2& lhs, const Vec2& rhs) {
    return norm(lhs - rhs) > kEps;
}

bool ring_has_live_bbox(const RingState& ring) {
    return ring.max_x >= ring.min_x && ring.max_y >= ring.min_y;
}

bool point_within_ring_bbox(const RingState& ring, const Vec2& p) {
    if (!ring_has_live_bbox(ring)) {
        return false;
    }
    return p.x + kEps >= ring.min_x && p.x - kEps <= ring.max_x &&
           p.y + kEps >= ring.min_y && p.y - kEps <= ring.max_y;
}

std::pair<int, int> point_to_cell(const RingState& ring, const Vec2& p) {
    const double cell = std::max(ring.grid_cell_size, kEps);
    const int cell_x = static_cast<int>(std::floor((p.x - ring.min_x) / cell));
    const int cell_y = static_cast<int>(std::floor((p.y - ring.min_y) / cell));
    return {cell_x, cell_y};
}

std::vector<long long> edge_cell_keys(const RingState& ring, const Vec2& a, const Vec2& b) {
    auto [min_cell_x, min_cell_y] = point_to_cell(ring, {std::min(a.x, b.x), std::min(a.y, b.y)});
    auto [max_cell_x, max_cell_y] = point_to_cell(ring, {std::max(a.x, b.x), std::max(a.y, b.y)});

    std::vector<long long> keys;
    keys.reserve(static_cast<std::size_t>(max_cell_x - min_cell_x + 1) *
                 static_cast<std::size_t>(max_cell_y - min_cell_y + 1));
    for (int cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x) {
        for (int cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y) {
            keys.push_back(cell_key(cell_x, cell_y));
        }
    }
    return keys;
}

void remove_edge_from_grid_internal(RingState& ring, int start_idx) {
    if (start_idx < 0 || start_idx >= static_cast<int>(ring.edge_grid_cells.size())) {
        return;
    }
    for (long long key : ring.edge_grid_cells[start_idx]) {
        auto it = ring.edge_grid.find(key);
        if (it == ring.edge_grid.end()) {
            continue;
        }
        auto& ids = it->second;
        ids.erase(std::remove(ids.begin(), ids.end(), start_idx), ids.end());
        if (ids.empty()) {
            ring.edge_grid.erase(it);
        }
    }
    ring.edge_grid_cells[start_idx].clear();
}

void insert_edge_into_grid_internal(RingState& ring, int start_idx) {
    if (start_idx < 0 || start_idx >= static_cast<int>(ring.nodes.size()) || !ring.nodes[start_idx].alive) {
        return;
    }

    const int end_idx = ring.nodes[start_idx].next;
    const auto keys = edge_cell_keys(ring, ring.nodes[start_idx].point, ring.nodes[end_idx].point);
    if (start_idx >= static_cast<int>(ring.edge_grid_cells.size())) {
        ring.edge_grid_cells.resize(static_cast<std::size_t>(start_idx) + 1);
    }
    ring.edge_grid_cells[start_idx] = keys;
    for (long long key : keys) {
        ring.edge_grid[key].push_back(start_idx);
    }
}

void rebuild_edge_grid_internal(RingState& ring) {
    ring.edge_grid.clear();
    ring.edge_grid_cells.clear();
    ring.edge_grid_cells.resize(ring.nodes.size());

    ring.min_x = std::numeric_limits<double>::infinity();
    ring.min_y = std::numeric_limits<double>::infinity();
    ring.max_x = -std::numeric_limits<double>::infinity();
    ring.max_y = -std::numeric_limits<double>::infinity();

    const auto points = alive_ring_points(ring);
    for (const Vec2& p : points) {
        ring.min_x = std::min(ring.min_x, p.x);
        ring.max_x = std::max(ring.max_x, p.x);
        ring.min_y = std::min(ring.min_y, p.y);
        ring.max_y = std::max(ring.max_y, p.y);
    }

    const double span_x = std::max(ring.max_x - ring.min_x, 1.0);
    const double span_y = std::max(ring.max_y - ring.min_y, 1.0);
    const double span = std::max(span_x, span_y);
    const double cells_per_axis =
        std::max(1.0, std::sqrt(static_cast<double>(std::max(ring.alive_count, 1))));
    ring.grid_cell_size = std::max(span / cells_per_axis, kEps);

    for (const EdgeRef& edge : alive_edges(ring)) {
        insert_edge_into_grid_internal(ring, edge.start_idx);
    }
    ring.edge_grid_ready = true;
}

bool edge_intersects_ring_edges(std::vector<RingState>& rings,
                                int ring_id,
                                int a,
                                int b,
                                int c,
                                int d,
                                const Vec2& p,
                                const Vec2& q) {
    std::unordered_set<long long> seen_edges;
    for (RingState& ring : rings) {
        ensure_edge_grid(ring);
        const auto keys = edge_cell_keys(ring, p, q);
        for (long long key : keys) {
            auto it = ring.edge_grid.find(key);
            if (it == ring.edge_grid.end()) {
                continue;
            }
            for (int start_idx : it->second) {
                const long long edge_id =
                    (static_cast<long long>(ring.ring_id) << 32) ^
                    static_cast<unsigned int>(start_idx);
                if (!seen_edges.insert(edge_id).second) {
                    continue;
                }
                const EdgeRef edge{ring.ring_id, start_idx, ring.nodes[start_idx].next};
                if (ring.ring_id == ring_id && same_ring_edge_adjacent_to_new_edge(edge, a, b, c, d)) {
                    continue;
                }
                const Vec2& c_pt = ring.nodes[edge.start_idx].point;
                const Vec2& d_pt = ring.nodes[edge.end_idx].point;
                if (!bounding_boxes_overlap(p, q, c_pt, d_pt)) {
                    continue;
                }
                if (segments_intersect(p, q, c_pt, d_pt)) {
                    return true;
                }
            }
        }
    }
    return false;
}

}  // namespace

/**
 * @copydoc CandidateCompare::operator()(const Candidate&, const Candidate&) const
 */
bool CandidateCompare::operator()(const Candidate& lhs, const Candidate& rhs) const {
    if (std::abs(lhs.displacement - rhs.displacement) > 1e-12) {
        return lhs.displacement > rhs.displacement;
    }
    if (lhs.ring_id != rhs.ring_id) {
        return lhs.ring_id > rhs.ring_id;
    }
    return lhs.b > rhs.b;
}

/**
 * @copydoc ensure_edge_grid(RingState&)
 */
void ensure_edge_grid(RingState& ring) {
    if (!ring.edge_grid_ready) {
        rebuild_edge_grid_internal(ring);
    }
}

/**
 * @copydoc update_edge_grid_after_collapse(RingState&, int, int, int, int)
 */
void update_edge_grid_after_collapse(RingState& ring, int a, int b, int c, int e) {
    if (!ring.edge_grid_ready) {
        return;
    }

    if (!point_within_ring_bbox(ring, ring.nodes[e].point)) {
        ring.edge_grid_ready = false;
        return;
    }

    remove_edge_from_grid_internal(ring, b);
    remove_edge_from_grid_internal(ring, c);
    remove_edge_from_grid_internal(ring, a);
    remove_edge_from_grid_internal(ring, e);

    insert_edge_into_grid_internal(ring, a);
    insert_edge_into_grid_internal(ring, e);
}

/**
 * @copydoc alive_ring_points(const RingState&)
 */
std::vector<Vec2> alive_ring_points(const RingState& ring) {
    std::vector<Vec2> points;
    if (ring.alive_count == 0 || ring.any_alive < 0) {
        return points;
    }

    points.reserve(static_cast<std::size_t>(ring.alive_count));
    int start = ring.any_alive;
    int current = start;
    int steps = 0;
    do {
        if (++steps > static_cast<int>(ring.nodes.size()) + 1) {
            throw std::runtime_error("Corrupt ring state: traversal did not close.");
        }
        const Node& node = ring.nodes[current];
        if (!node.alive) {
            throw std::runtime_error("Corrupt ring state: encountered dead node while traversing.");
        }
        points.push_back(node.point);
        current = node.next;
    } while (current != start);

    return points;
}

/**
 * @copydoc polygon_is_valid(const std::vector<RingState>&)
 */
bool polygon_is_valid(const std::vector<RingState>& rings) {
    if (rings.empty()) {
        return false;
    }

    std::vector<std::vector<Vec2>> all_points;
    all_points.reserve(rings.size());
    for (const RingState& ring : rings) {
        auto pts = alive_ring_points(ring);
        if (pts.size() < 3) {
            return false;
        }
        for (std::size_t i = 0; i < pts.size(); ++i) {
            if (norm(pts[(i + 1) % pts.size()] - pts[i]) <= kEps) {
                return false;
            }
        }
        all_points.push_back(std::move(pts));
    }

    for (std::size_t r = 0; r < all_points.size(); ++r) {
        const auto& ring = all_points[r];
        const std::size_t n = ring.size();
        for (std::size_t i = 0; i < n; ++i) {
            const Vec2& a = ring[i];
            const Vec2& b = ring[(i + 1) % n];
            for (std::size_t j = i + 1; j < n; ++j) {
                if (same_ring_segments_are_adjacent(n, i, j)) {
                    continue;
                }
                const Vec2& c = ring[j];
                const Vec2& d = ring[(j + 1) % n];
                if (segments_intersect(a, b, c, d)) {
                    return false;
                }
            }
        }
    }

    for (std::size_t r1 = 0; r1 < all_points.size(); ++r1) {
        for (std::size_t r2 = r1 + 1; r2 < all_points.size(); ++r2) {
            const auto& ring1 = all_points[r1];
            const auto& ring2 = all_points[r2];
            for (std::size_t i = 0; i < ring1.size(); ++i) {
                const Vec2& a = ring1[i];
                const Vec2& b = ring1[(i + 1) % ring1.size()];
                for (std::size_t j = 0; j < ring2.size(); ++j) {
                    const Vec2& c = ring2[j];
                    const Vec2& d = ring2[(j + 1) % ring2.size()];
                    if (segments_intersect(a, b, c, d)) {
                        return false;
                    }
                }
            }
        }
    }

    const auto& outer = all_points[0];
    for (std::size_t hole_id = 1; hole_id < all_points.size(); ++hole_id) {
        const auto& hole = all_points[hole_id];
        if (!point_in_ring_strict(outer, hole.front())) {
            return false;
        }
        for (std::size_t other = 1; other < all_points.size(); ++other) {
            if (hole_id == other) {
                continue;
            }
            if (point_in_ring_strict(all_points[other], hole.front())) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @copydoc collapse_preserves_validity(std::vector<RingState>&, int, int, int, const Vec2&)
 */
bool collapse_preserves_validity(std::vector<RingState>& rings, int ring_id, int a, int d, const Vec2& e) {
    if (ring_id < 0 || ring_id >= static_cast<int>(rings.size())) {
        return false;
    }

    ensure_edge_grid(rings[ring_id]);
    const RingState& ring = rings[ring_id];
    if (!ring.nodes[a].alive || !ring.nodes[d].alive) {
        return false;
    }
    const int b = ring.nodes[a].next;
    const int c = ring.nodes[d].prev;
    if (b == d || c == a || b == c) {
        return false;
    }

    const int a_prev = ring.nodes[a].prev;
    const int d_next = ring.nodes[d].next;
    const Vec2& a_pt = ring.nodes[a].point;
    const Vec2& d_pt = ring.nodes[d].point;

    if (!points_are_distinct(a_pt, e) || !points_are_distinct(e, d_pt)) {
        return false;
    }
    if (!points_are_distinct(ring.nodes[a_prev].point, a_pt) ||
        !points_are_distinct(d_pt, ring.nodes[d_next].point)) {
        return false;
    }

    if (edge_intersects_ring_edges(rings, ring_id, a, b, c, d, a_pt, e)) {
        return false;
    }
    if (edge_intersects_ring_edges(rings, ring_id, a, b, c, d, e, d_pt)) {
        return false;
    }

    if (rings.size() == 1) {
        return true;
    }

    PolygonData tentative;
    tentative.rings = rings;
    RingState& work_ring = tentative.rings[ring_id];
    const int e_idx = static_cast<int>(work_ring.nodes.size());
    work_ring.nodes.push_back(Node{e, a, d, true, work_ring.generation + 1});
    work_ring.nodes[a].next = e_idx;
    work_ring.nodes[d].prev = e_idx;
    work_ring.nodes[b].alive = false;
    work_ring.nodes[c].alive = false;
    work_ring.alive_count -= 1;
    work_ring.any_alive = e_idx;

    if (ring_id == 0) {
        const auto outer = alive_ring_points(work_ring);
        for (std::size_t hole_id = 1; hole_id < tentative.rings.size(); ++hole_id) {
            const auto hole = alive_ring_points(tentative.rings[hole_id]);
            if (!point_in_ring_strict(outer, hole.front())) {
                return false;
            }
        }
    } else {
        const auto outer = alive_ring_points(tentative.rings[0]);
        const auto hole = alive_ring_points(work_ring);
        if (!point_in_ring_strict(outer, hole.front())) {
            return false;
        }
        for (std::size_t other = 1; other < tentative.rings.size(); ++other) {
            if (static_cast<int>(other) == ring_id) {
                continue;
            }
            const auto other_hole = alive_ring_points(tentative.rings[other]);
            if (point_in_ring_strict(other_hole, hole.front()) ||
                point_in_ring_strict(hole, other_hole.front())) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @copydoc total_signed_area(const std::vector<RingState>&)
 */
double total_signed_area(const std::vector<RingState>& rings) {
    double area = 0.0;
    for (const auto& ring : rings) {
        area += signed_area_of_ring_points(alive_ring_points(ring));
    }
    return area;
}

/**
 * @copydoc total_vertex_count(const std::vector<RingState>&)
 */
int total_vertex_count(const std::vector<RingState>& rings) {
    int total = 0;
    for (const auto& ring : rings) {
        total += ring.alive_count;
    }
    return total;
}

}  // namespace polygon_simplification
