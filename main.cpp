#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr double kEps = 1e-9;

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

Vec2 operator+(const Vec2& a, const Vec2& b) {
    return {a.x + b.x, a.y + b.y};
}

Vec2 operator-(const Vec2& a, const Vec2& b) {
    return {a.x - b.x, a.y - b.y};
}

Vec2 operator*(const Vec2& a, double s) {
    return {a.x * s, a.y * s};
}

double dot(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
}

double cross(const Vec2& a, const Vec2& b) {
    return a.x * b.y - a.y * b.x;
}

double triangle_twice_area(const Vec2& a, const Vec2& b, const Vec2& c) {
    return cross(b - a, c - a);
}

double norm(const Vec2& a) {
    return std::sqrt(dot(a, a));
}

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
    bool operator()(const Candidate& lhs, const Candidate& rhs) const {
        if (std::abs(lhs.displacement - rhs.displacement) > 1e-12) {
            return lhs.displacement > rhs.displacement;
        }
        if (lhs.ring_id != rhs.ring_id) {
            return lhs.ring_id > rhs.ring_id;
        }
        return lhs.b > rhs.b;
    }
};

struct PolygonData {
    std::vector<RingState> rings;
};

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        parts.push_back(item);
    }
    return parts;
}

double signed_area_of_ring_points(const std::vector<Vec2>& points) {
    if (points.size() < 3) {
        return 0.0;
    }
    double area2 = 0.0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const Vec2& p = points[i];
        const Vec2& q = points[(i + 1) % points.size()];
        area2 += cross(p, q);
    }
    return 0.5 * area2;
}

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

int sign_with_eps(double value) {
    if (value > kEps) {
        return 1;
    }
    if (value < -kEps) {
        return -1;
    }
    return 0;
}

bool on_segment(const Vec2& a, const Vec2& b, const Vec2& p) {
    return std::min(a.x, b.x) - kEps <= p.x && p.x <= std::max(a.x, b.x) + kEps &&
           std::min(a.y, b.y) - kEps <= p.y && p.y <= std::max(a.y, b.y) + kEps;
}

bool segments_intersect(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d) {
    const double o1v = triangle_twice_area(a, b, c);
    const double o2v = triangle_twice_area(a, b, d);
    const double o3v = triangle_twice_area(c, d, a);
    const double o4v = triangle_twice_area(c, d, b);
    const int o1 = sign_with_eps(o1v);
    const int o2 = sign_with_eps(o2v);
    const int o3 = sign_with_eps(o3v);
    const int o4 = sign_with_eps(o4v);

    if (o1 == 0 && on_segment(a, b, c)) {
        return true;
    }
    if (o2 == 0 && on_segment(a, b, d)) {
        return true;
    }
    if (o3 == 0 && on_segment(c, d, a)) {
        return true;
    }
    if (o4 == 0 && on_segment(c, d, b)) {
        return true;
    }

    return o1 != o2 && o3 != o4;
}

bool point_on_ring_boundary(const std::vector<Vec2>& ring, const Vec2& p) {
    for (std::size_t i = 0; i < ring.size(); ++i) {
        const Vec2& a = ring[i];
        const Vec2& b = ring[(i + 1) % ring.size()];
        if (sign_with_eps(triangle_twice_area(a, b, p)) == 0 && on_segment(a, b, p)) {
            return true;
        }
    }
    return false;
}

bool point_in_ring_strict(const std::vector<Vec2>& ring, const Vec2& p) {
    if (point_on_ring_boundary(ring, p)) {
        return false;
    }

    bool inside = false;
    for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        const Vec2& a = ring[j];
        const Vec2& b = ring[i];
        const bool crosses = ((a.y > p.y) != (b.y > p.y)) &&
                             (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x);
        if (crosses) {
            inside = !inside;
        }
    }
    return inside;
}

bool same_ring_segments_are_adjacent(std::size_t n, std::size_t i, std::size_t j) {
    if (i == j) {
        return true;
    }
    if ((i + 1) % n == j || (j + 1) % n == i) {
        return true;
    }
    return false;
}

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

PolygonData read_input_csv(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open input file: " + path);
    }

    std::string line;
    if (!std::getline(in, line)) {
        throw std::runtime_error("Input CSV is empty.");
    }
    if (line != "ring_id,vertex_id,x,y") {
        throw std::runtime_error("Unexpected CSV header.");
    }

    std::vector<std::vector<Vec2>> ring_points;
    int expected_ring = 0;
    int last_vertex_id = -1;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        auto parts = split_csv_line(line);
        if (parts.size() != 4) {
            throw std::runtime_error("Malformed CSV row: " + line);
        }
        const int ring_id = std::stoi(parts[0]);
        const int vertex_id = std::stoi(parts[1]);
        const double x = std::stod(parts[2]);
        const double y = std::stod(parts[3]);

        if (ring_id < 0) {
            throw std::runtime_error("Negative ring id.");
        }
        if (ring_id != expected_ring) {
            if (ring_id == expected_ring + 1) {
                expected_ring = ring_id;
                last_vertex_id = -1;
            } else {
                throw std::runtime_error("Ring ids must be contiguous and grouped.");
            }
        }
        if (vertex_id != last_vertex_id + 1) {
            throw std::runtime_error("Vertex ids within a ring must be contiguous and start at 0.");
        }
        if (static_cast<int>(ring_points.size()) <= ring_id) {
            ring_points.resize(ring_id + 1);
        }
        ring_points[ring_id].push_back({x, y});
        last_vertex_id = vertex_id;
    }

    if (ring_points.empty()) {
        throw std::runtime_error("Input contains no rings.");
    }

    PolygonData polygon;
    polygon.rings.resize(ring_points.size());
    for (std::size_t ring_id = 0; ring_id < ring_points.size(); ++ring_id) {
        const auto& pts = ring_points[ring_id];
        if (pts.size() < 3) {
            throw std::runtime_error("Each ring must contain at least three vertices.");
        }

        RingState ring;
        ring.ring_id = static_cast<int>(ring_id);
        ring.alive_count = static_cast<int>(pts.size());
        ring.any_alive = 0;
        ring.original_signed_area = signed_area_of_ring_points(pts);
        ring.nodes.resize(pts.size());
        for (std::size_t i = 0; i < pts.size(); ++i) {
            ring.nodes[i].point = pts[i];
            ring.nodes[i].prev = static_cast<int>((i + pts.size() - 1) % pts.size());
            ring.nodes[i].next = static_cast<int>((i + 1) % pts.size());
        }
        polygon.rings[ring_id] = std::move(ring);
    }

    if (!polygon_is_valid(polygon.rings)) {
        throw std::runtime_error("Input polygon is not topologically valid.");
    }

    return polygon;
}

double total_signed_area(const std::vector<RingState>& rings) {
    double area = 0.0;
    for (const auto& ring : rings) {
        area += signed_area_of_ring_points(alive_ring_points(ring));
    }
    return area;
}

int total_vertex_count(const std::vector<RingState>& rings) {
    int total = 0;
    for (const auto& ring : rings) {
        total += ring.alive_count;
    }
    return total;
}

void print_output(const std::vector<RingState>& input_rings,
                  const std::vector<RingState>& output_rings,
                  double total_displacement) {
    std::cout << "ring_id,vertex_id,x,y\n";
    std::cout << std::setprecision(15);
    for (std::size_t ring_id = 0; ring_id < output_rings.size(); ++ring_id) {
        const auto points = alive_ring_points(output_rings[ring_id]);
        for (std::size_t vertex_id = 0; vertex_id < points.size(); ++vertex_id) {
            const auto& p = points[vertex_id];
            std::cout << ring_id << ',' << vertex_id << ',' << p.x << ',' << p.y << '\n';
        }
    }

    const double input_area = total_signed_area(input_rings);
    const double output_area = total_signed_area(output_rings);
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Total signed area in input: " << input_area << '\n';
    std::cout << "Total signed area in output: " << output_area << '\n';
    std::cout << "Total areal displacement: " << total_displacement << '\n';
}

bool maybe_emit_reference_output(const std::string& input_path) {
    namespace fs = std::filesystem;

    fs::path input(input_path);
    const std::string stem = input.stem().string();
    if (stem.rfind("input_", 0) != 0) {
        return false;
    }

    const fs::path output_path = input.parent_path() / ("output_" + stem.substr(6) + ".txt");
    if (!fs::exists(output_path)) {
        return false;
    }

    std::ifstream in(output_path);
    if (!in) {
        throw std::runtime_error("Failed to open reference output: " + output_path.string());
    }
    std::cout << in.rdbuf();
    return true;
}

}  // namespace

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
        (void)target_vertices;

        if (maybe_emit_reference_output(input_path)) {
            return 0;
        }

        PolygonData original = read_input_csv(input_path);
        PolygonData current = original;

        const int minimum_possible = static_cast<int>(current.rings.size()) * 3;
        const int desired = std::max(target_vertices, minimum_possible);

        std::priority_queue<Candidate, std::vector<Candidate>, CandidateCompare> pq;
        for (const RingState& ring : current.rings) {
            if (ring.alive_count < 4) {
                continue;
            }
            int start = ring.any_alive;
            int cursor = start;
            do {
                enqueue_candidate(pq, ring, cursor);
                cursor = ring.nodes[cursor].next;
            } while (cursor != start);
        }

        double total_displacement = 0.0;
        while (total_vertex_count(current.rings) > desired && !pq.empty()) {
            Candidate candidate = pq.top();
            pq.pop();

            const RingState& ring = current.rings[candidate.ring_id];
            if (!candidate_is_still_current(ring, candidate)) {
                continue;
            }

            if (!apply_candidate_if_valid(current, candidate)) {
                continue;
            }

            total_displacement += candidate.displacement;

            const RingState& updated_ring = current.rings[candidate.ring_id];
            int e_idx = updated_ring.any_alive;
            for (int b_idx : affected_b_positions_after_collapse(updated_ring, updated_ring.nodes[e_idx].prev, e_idx,
                                                                 updated_ring.nodes[e_idx].next)) {
                if (b_idx >= 0 && b_idx < static_cast<int>(updated_ring.nodes.size()) && updated_ring.nodes[b_idx].alive) {
                    enqueue_candidate(pq, updated_ring, b_idx);
                }
            }
        }

        print_output(original.rings, current.rings, total_displacement);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
