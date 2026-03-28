#include "polygon.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace polygon_simplification {

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
