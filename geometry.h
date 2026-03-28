#ifndef POLYGON_SIMPLIFICATION_GEOMETRY_H
#define POLYGON_SIMPLIFICATION_GEOMETRY_H

#include <cstddef>
#include <vector>

namespace polygon_simplification {

inline constexpr double kEps = 1e-9;

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

Vec2 operator+(const Vec2& a, const Vec2& b);
Vec2 operator-(const Vec2& a, const Vec2& b);
Vec2 operator*(const Vec2& a, double s);

double dot(const Vec2& a, const Vec2& b);
double cross(const Vec2& a, const Vec2& b);
double triangle_twice_area(const Vec2& a, const Vec2& b, const Vec2& c);
double norm(const Vec2& a);
int sign_with_eps(double value);

double signed_area_of_ring_points(const std::vector<Vec2>& points);
bool on_segment(const Vec2& a, const Vec2& b, const Vec2& p);
bool segments_intersect(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d);
bool point_on_ring_boundary(const std::vector<Vec2>& ring, const Vec2& p);
bool point_in_ring_strict(const std::vector<Vec2>& ring, const Vec2& p);
bool same_ring_segments_are_adjacent(std::size_t n, std::size_t i, std::size_t j);

}  // namespace polygon_simplification

#endif
