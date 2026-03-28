#ifndef POLYGON_SIMPLIFICATION_GEOMETRY_H
#define POLYGON_SIMPLIFICATION_GEOMETRY_H

#include <cstddef>
#include <vector>

namespace polygon_simplification {

/**
 * @brief Numerical tolerance used by geometric predicates.
 */
inline constexpr double kEps = 1e-9;

/**
 * @brief Two-dimensional point or vector.
 */
struct Vec2 {
    /** @brief X coordinate. */
    double x = 0.0;
    /** @brief Y coordinate. */
    double y = 0.0;
};

/**
 * @brief Adds two vectors component-wise.
 * @param a Left operand.
 * @param b Right operand.
 * @return The sum vector.
 */
Vec2 operator+(const Vec2& a, const Vec2& b);

/**
 * @brief Subtracts two vectors component-wise.
 * @param a Left operand.
 * @param b Right operand.
 * @return The difference vector.
 */
Vec2 operator-(const Vec2& a, const Vec2& b);

/**
 * @brief Multiplies a vector by a scalar.
 * @param a Vector operand.
 * @param s Scalar multiplier.
 * @return The scaled vector.
 */
Vec2 operator*(const Vec2& a, double s);

/**
 * @brief Computes the dot product of two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return Dot product of @p a and @p b.
 */
double dot(const Vec2& a, const Vec2& b);

/**
 * @brief Computes the 2D cross product magnitude of two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return Signed scalar cross product.
 */
double cross(const Vec2& a, const Vec2& b);

/**
 * @brief Computes twice the signed area of triangle ABC.
 * @param a First triangle vertex.
 * @param b Second triangle vertex.
 * @param c Third triangle vertex.
 * @return Twice the signed area.
 */
double triangle_twice_area(const Vec2& a, const Vec2& b, const Vec2& c);

/**
 * @brief Computes the Euclidean norm of a vector.
 * @param a Input vector.
 * @return Vector length.
 */
double norm(const Vec2& a);

/**
 * @brief Converts a floating-point value to a three-way sign using @ref kEps.
 * @param value Input value.
 * @return `1` for positive, `-1` for negative, or `0` when near zero.
 */
int sign_with_eps(double value);

/**
 * @brief Computes the signed area of a closed ring.
 * @param points Ring vertices in order.
 * @return Signed area of the polygonal ring.
 */
double signed_area_of_ring_points(const std::vector<Vec2>& points);

/**
 * @brief Tests whether a point lies on a closed segment.
 * @param a Segment start.
 * @param b Segment end.
 * @param p Point to test.
 * @return `true` if @p p lies on segment AB within tolerance.
 */
bool on_segment(const Vec2& a, const Vec2& b, const Vec2& p);

/**
 * @brief Tests whether two closed segments intersect.
 * @param a First endpoint of the first segment.
 * @param b Second endpoint of the first segment.
 * @param c First endpoint of the second segment.
 * @param d Second endpoint of the second segment.
 * @return `true` when the segments intersect or touch.
 */
bool segments_intersect(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d);

/**
 * @brief Tests whether a point lies on the boundary of a ring.
 * @param ring Ring vertices in order.
 * @param p Point to test.
 * @return `true` if @p p lies on any ring edge.
 */
bool point_on_ring_boundary(const std::vector<Vec2>& ring, const Vec2& p);

/**
 * @brief Tests whether a point lies strictly inside a ring.
 * @param ring Ring vertices in order.
 * @param p Point to test.
 * @return `true` if @p p is strictly inside the ring and not on its boundary.
 */
bool point_in_ring_strict(const std::vector<Vec2>& ring, const Vec2& p);

/**
 * @brief Tests whether two edges in the same ring are identical or adjacent.
 * @param n Number of edges in the ring.
 * @param i Index of the first edge.
 * @param j Index of the second edge.
 * @return `true` if the edges share a vertex or are the same edge.
 */
bool same_ring_segments_are_adjacent(std::size_t n, std::size_t i, std::size_t j);

}  // namespace polygon_simplification

#endif
