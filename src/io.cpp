#include "io.h"

#include "geometry.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace polygon_simplification {

namespace {

/**
 * @brief Removes a single trailing carriage return from a text line.
 * @param line Input line.
 * @return The normalized line.
 */
std::string trim_trailing_carriage_return(std::string line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

/**
 * @brief Splits a simple comma-separated line into fields.
 * @param line Input CSV row without embedded quoted commas.
 * @return Parsed field strings.
 */
std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        parts.push_back(item);
    }
    return parts;
}

}  // namespace

/**
 * @copydoc read_input_csv(const std::string&)
 */
PolygonData read_input_csv(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open input file: " + path);
    }

    std::string line;
    if (!std::getline(in, line)) {
        throw std::runtime_error("Input CSV is empty.");
    }
    line = trim_trailing_carriage_return(std::move(line));
    if (line != "ring_id,vertex_id,x,y") {
        throw std::runtime_error("Unexpected CSV header.");
    }

    std::vector<std::vector<Vec2>> ring_points;
    int expected_ring = 0;
    int last_vertex_id = -1;
    while (std::getline(in, line)) {
        line = trim_trailing_carriage_return(std::move(line));
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

    return polygon;
}

/**
 * @copydoc print_output(const std::vector<RingState>&, const std::vector<RingState>&, double)
 */
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

/**
 * @copydoc maybe_emit_reference_output(const std::string&)
 */
bool maybe_emit_reference_output(const std::string& input_path) {
    (void)input_path;
    return false;
}

}  // namespace polygon_simplification
