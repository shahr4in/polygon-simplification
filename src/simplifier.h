#ifndef POLYGON_SIMPLIFICATION_SIMPLIFIER_H
#define POLYGON_SIMPLIFICATION_SIMPLIFIER_H

#include "polygon.h"

#include <optional>
#include <queue>
#include <vector>

namespace polygon_simplification {

/**
 * @brief Builds the greedy collapse candidate centered at a given vertex B.
 * @param ring Ring containing the candidate.
 * @param b_idx Index of vertex B in the local chain A-B-C-D.
 * @return The computed candidate, or `std::nullopt` when the local configuration is invalid.
 */
std::optional<Candidate> compute_candidate(const RingState& ring, int b_idx);

/**
 * @brief Checks whether a previously computed candidate still matches the current ring state.
 * @param ring Ring to validate against.
 * @param candidate Candidate to verify.
 * @return `true` if the involved nodes are still alive, unchanged, and adjacent in the same order.
 */
bool candidate_is_still_current(const RingState& ring, const Candidate& candidate);

/**
 * @brief Computes and queues a candidate when possible.
 * @param pq Priority queue of candidate collapses.
 * @param ring Ring containing the candidate.
 * @param b_idx Index of the middle-left vertex B.
 */
void enqueue_candidate(std::priority_queue<Candidate, std::vector<Candidate>, CandidateCompare>& pq,
                       const RingState& ring,
                       int b_idx);

/**
 * @brief Returns the nearby B positions whose candidates may change after a collapse.
 * @param ring Updated ring.
 * @param a Index of A in the new local chain.
 * @param e Index of the newly inserted replacement point E.
 * @param d Index of D in the new local chain.
 * @return Unique indices whose local neighborhoods should be recomputed.
 */
std::vector<int> affected_b_positions_after_collapse(const RingState& ring, int a, int e, int d);

/**
 * @brief Applies a candidate collapse if the resulting polygon remains valid.
 * @param polygon Polygon to update.
 * @param candidate Candidate collapse to apply.
 * @return `true` if the collapse was accepted and committed.
 */
bool apply_candidate_if_valid(PolygonData& polygon, const Candidate& candidate);

}  // namespace polygon_simplification

#endif
