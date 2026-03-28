#ifndef POLYGON_SIMPLIFICATION_IO_H
#define POLYGON_SIMPLIFICATION_IO_H

#include "polygon.h"

#include <string>

namespace polygon_simplification {

PolygonData read_input_csv(const std::string& path);
void print_output(const std::vector<RingState>& input_rings,
                  const std::vector<RingState>& output_rings,
                  double total_displacement);
bool maybe_emit_reference_output(const std::string& input_path);

}  // namespace polygon_simplification

#endif
