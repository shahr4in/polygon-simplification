#include "io.h"
#include "polygon.h"
#include "simplifier.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Program entry point.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 * @return Exit status code.
 */
int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: ./simplify <input_file.csv> <target_vertices>\n";
            return 1;
        }

        const std::string input_path = argv[1];
        const int target_vertices = std::stoi(argv[2]);
        if (target_vertices < 0) {
            throw std::runtime_error("Target vertex count must be non-negative.");
        }

        polygon_simplification::PolygonData original = polygon_simplification::read_input_csv(input_path);
        polygon_simplification::PolygonData current = original;

        const int minimum_possible = static_cast<int>(current.rings.size()) * 3;
        const int desired = std::max(target_vertices, minimum_possible);

        std::priority_queue<polygon_simplification::Candidate,
                            std::vector<polygon_simplification::Candidate>,
                            polygon_simplification::CandidateCompare>
            pq;
        for (const polygon_simplification::RingState& ring : current.rings) {
            if (ring.alive_count < 4) {
                continue;
            }
            int start = ring.any_alive;
            int cursor = start;
            do {
                polygon_simplification::enqueue_candidate(pq, ring, cursor);
                cursor = ring.nodes[cursor].next;
            } while (cursor != start);
        }

        double total_displacement = 0.0;
        while (polygon_simplification::total_vertex_count(current.rings) > desired && !pq.empty()) {
            polygon_simplification::Candidate candidate = pq.top();
            pq.pop();

            const polygon_simplification::RingState& ring = current.rings[candidate.ring_id];
            if (!polygon_simplification::candidate_is_still_current(ring, candidate)) {
                continue;
            }

            if (!polygon_simplification::apply_candidate_if_valid(current, candidate)) {
                continue;
            }

            total_displacement += candidate.displacement;

            const polygon_simplification::RingState& updated_ring = current.rings[candidate.ring_id];
            const int e_idx = updated_ring.any_alive;
            for (int b_idx : polygon_simplification::affected_b_positions_after_collapse(
                     updated_ring, updated_ring.nodes[e_idx].prev, e_idx, updated_ring.nodes[e_idx].next)) {
                if (b_idx >= 0 &&
                    b_idx < static_cast<int>(updated_ring.nodes.size()) &&
                    updated_ring.nodes[b_idx].alive) {
                    polygon_simplification::enqueue_candidate(pq, updated_ring, b_idx);
                }
            }
        }

        polygon_simplification::print_output(original.rings, current.rings, total_displacement);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
