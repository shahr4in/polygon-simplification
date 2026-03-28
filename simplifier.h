#ifndef POLYGON_SIMPLIFICATION_SIMPLIFIER_H
#define POLYGON_SIMPLIFICATION_SIMPLIFIER_H

#include "polygon.h"

#include <optional>
#include <queue>
#include <vector>

namespace polygon_simplification {

std::optional<Candidate> compute_candidate(const RingState& ring, int b_idx);
bool candidate_is_still_current(const RingState& ring, const Candidate& candidate);
void enqueue_candidate(std::priority_queue<Candidate, std::vector<Candidate>, CandidateCompare>& pq,
                       const RingState& ring,
                       int b_idx);
std::vector<int> affected_b_positions_after_collapse(const RingState& ring, int a, int e, int d);
bool apply_candidate_if_valid(PolygonData& polygon, const Candidate& candidate);

}  // namespace polygon_simplification

#endif
