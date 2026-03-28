#include "simplifier.h"

#include "geometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace polygon_simplification {

std::optional<Candidate> compute_candidate(const RingState& ring, int b_idx) {
    if (!ring.nodes[b_idx].alive || ring.alive_count < 4) {
        return std::nullopt;
    }

    const Node& b = ring.nodes[b_idx];
    const int a_idx = b.prev;
    const int c_idx = b.next;
    const int d_idx = ring.nodes[c_idx].next;

    if (a_idx == b_idx || c_idx == b_idx || d_idx == b_idx || a_idx == c_idx || a_idx == d_idx || c_idx == d_idx) {
        return std::nullopt;
    }
    if (!ring.nodes[a_idx].alive || !ring.nodes[c_idx].alive || !ring.nodes[d_idx].alive) {
        return std::nullopt;
    }

    const Vec2& a = ring.nodes[a_idx].point;
    const Vec2& bpt = b.point;
    const Vec2& c = ring.nodes[c_idx].point;
    const Vec2& d = ring.nodes[d_idx].point;

    const Vec2 ad = d - a;
    const double ad_len = norm(ad);
    if (ad_len <= kEps) {
        return std::nullopt;
    }

    const double k = cross(a, bpt) + cross(bpt, c) + cross(c, d);
    const Vec2 perp{-ad.y, ad.x};
    const Vec2 e0 = perp * (-k / dot(ad, ad));
    const Vec2 u = ad * (1.0 / ad_len);

    struct Term {
        double alpha;
        double beta;
    };

    const std::array<std::pair<Vec2, Vec2>, 3> segments{{{a, bpt}, {bpt, c}, {c, d}}};
    std::vector<std::pair<double, double>> roots;
    std::vector<Term> terms;
    terms.reserve(3);
    for (const auto& seg : segments) {
        const Vec2 p = seg.first;
        const Vec2 q = seg.second;
        const Vec2 s = q - p;
        const double alpha = cross(s, u);
        const double beta = cross(s, e0 - p);
        terms.push_back({alpha, beta});
        if (std::abs(alpha) > kEps) {
            roots.push_back({-beta / alpha, std::abs(alpha)});
        }
    }

    double t = 0.0;
    if (!roots.empty()) {
        std::sort(roots.begin(), roots.end(),
                  [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
        double total_weight = 0.0;
        for (const auto& [_, weight] : roots) {
            total_weight += weight;
        }
        double cumulative = 0.0;
        for (std::size_t i = 0; i < roots.size(); ++i) {
            cumulative += roots[i].second;
            if (cumulative + kEps >= 0.5 * total_weight) {
                if (std::abs(cumulative - 0.5 * total_weight) <= 1e-12 && i + 1 < roots.size()) {
                    t = 0.5 * (roots[i].first + roots[i + 1].first);
                } else {
                    t = roots[i].first;
                }
                break;
            }
        }
    }

    const Vec2 e = e0 + u * t;
    double displacement2 = 0.0;
    for (const Term& term : terms) {
        displacement2 += std::abs(term.alpha * t + term.beta);
    }

    Candidate candidate;
    candidate.ring_id = ring.ring_id;
    candidate.a = a_idx;
    candidate.b = b_idx;
    candidate.c = c_idx;
    candidate.d = d_idx;
    candidate.e = e;
    candidate.displacement = 0.5 * displacement2;
    candidate.versions = {
        ring.nodes[a_idx].version,
        ring.nodes[b_idx].version,
        ring.nodes[c_idx].version,
        ring.nodes[d_idx].version,
    };
    return candidate;
}

bool candidate_is_still_current(const RingState& ring, const Candidate& candidate) {
    const std::array<int, 4> ids{candidate.a, candidate.b, candidate.c, candidate.d};
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const int id = ids[i];
        if (id < 0 || id >= static_cast<int>(ring.nodes.size())) {
            return false;
        }
        const Node& node = ring.nodes[id];
        if (!node.alive || node.version != candidate.versions[i]) {
            return false;
        }
    }
    return ring.nodes[candidate.a].next == candidate.b &&
           ring.nodes[candidate.b].next == candidate.c &&
           ring.nodes[candidate.c].next == candidate.d &&
           ring.nodes[candidate.b].prev == candidate.a &&
           ring.nodes[candidate.c].prev == candidate.b &&
           ring.nodes[candidate.d].prev == candidate.c;
}

void enqueue_candidate(std::priority_queue<Candidate, std::vector<Candidate>, CandidateCompare>& pq,
                       const RingState& ring,
                       int b_idx) {
    auto candidate = compute_candidate(ring, b_idx);
    if (candidate.has_value()) {
        pq.push(*candidate);
    }
}

std::vector<int> affected_b_positions_after_collapse(const RingState& ring, int a, int e, int d) {
    std::vector<int> ids;
    const int a_prev = ring.nodes[a].prev;
    const int d_next = ring.nodes[d].next;
    ids.push_back(a_prev);
    ids.push_back(a);
    ids.push_back(e);
    ids.push_back(d);
    ids.push_back(d_next);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

bool apply_candidate_if_valid(PolygonData& polygon, const Candidate& candidate) {
    RingState& ring = polygon.rings[candidate.ring_id];
    if (!candidate_is_still_current(ring, candidate)) {
        return false;
    }
    if (ring.alive_count <= 3) {
        return false;
    }

    PolygonData tentative = polygon;
    RingState& work_ring = tentative.rings[candidate.ring_id];

    if (work_ring.alive_count - 1 < 3) {
        return false;
    }

    const int e_idx = static_cast<int>(work_ring.nodes.size());
    work_ring.nodes.push_back(Node{candidate.e, candidate.a, candidate.d, true, work_ring.generation + 1});

    work_ring.nodes[candidate.a].next = e_idx;
    work_ring.nodes[candidate.a].version++;
    work_ring.nodes[candidate.d].prev = e_idx;
    work_ring.nodes[candidate.d].version++;
    work_ring.nodes[candidate.b].alive = false;
    work_ring.nodes[candidate.b].version++;
    work_ring.nodes[candidate.c].alive = false;
    work_ring.nodes[candidate.c].version++;
    work_ring.alive_count -= 1;
    work_ring.any_alive = e_idx;
    work_ring.generation++;

    if (!polygon_is_valid(tentative.rings)) {
        return false;
    }

    polygon = std::move(tentative);
    return true;
}

}  // namespace polygon_simplification
