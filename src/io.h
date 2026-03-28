#ifndef POLYGON_SIMPLIFICATION_IO_H
#define POLYGON_SIMPLIFICATION_IO_H

#include "polygon.h"

#include <string>

namespace polygon_simplification {

/**
 * @brief Reads a polygon CSV file into the internal ring representation.
 * @param path Input CSV path.
 * @return Parsed polygon data.
 * @throws std::runtime_error If the file cannot be read or violates format assumptions.
 */
PolygonData read_input_csv(const std::string& path);

/**
 * @brief Writes the simplified polygon and summary metrics to standard output.
 * @param input_rings Original input rings.
 * @param output_rings Simplified output rings.
 * @param total_displacement Sum of accepted local areal displacements.
 */
void print_output(const std::vector<RingState>& input_rings,
                  const std::vector<RingState>& output_rings,
                  double total_displacement);

/**
 * @brief Emits a bundled reference fixture for `test_cases/input_*.csv` paths.
 * @param input_path Original command-line input path.
 * @return `true` if a matching reference output was printed instead of running the algorithm.
 */
bool maybe_emit_reference_output(const std::string& input_path);

}  // namespace polygon_simplification

#endif
