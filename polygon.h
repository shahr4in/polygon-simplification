#ifndef POLYGON_SIMPLIFICATION_POLYGON_H
#define POLYGON_SIMPLIFICATION_POLYGON_H

#include "geometry.h"

#include <array>
#include <cstdint>
#include <vector>

namespace polygon_simplification {

struct Node {
    Vec2 point;
    int prev = -1;
    int next = -1;
    bool alive = true;
    std::uint64_t version = 0;
};

struct RingState {
    int ring_id = -1;
    std::vector<Node> nodes;
    int any_alive = -1;
    int alive_count = 0;
    std::uint64_t generation = 0;
    double original_signed_area = 0.0;
};

struct Candidate {
    int ring_id = -1;
    int b = -1;
    int c = -1;
    int a = -1;
    int d = -1;
    Vec2 e;
    double displacement = 0.0;
    std::array<std::uint64_t, 4> versions{};
};

struct CandidateCompare {
    bool operator()(const Candidate& lhs, const Candidate& rhs) const;
};

struct PolygonData {
    std::vector<RingState> rings;
};

std::vector<Vec2> alive_ring_points(const RingState& ring);
double total_signed_area(const std::vector<RingState>& rings);
int total_vertex_count(const std::vector<RingState>& rings);
bool polygon_is_valid(const std::vector<RingState>& rings);

}  // namespace polygon_simplification

#endif
