#include "geometry.h"

#include <algorithm>
#include <cmath>

namespace polygon_simplification {

/**
 * @copydoc operator+(const Vec2&, const Vec2&)
 */
Vec2 operator+(const Vec2& a, const Vec2& b) {
    return {a.x + b.x, a.y + b.y};
}

/**
 * @copydoc operator-(const Vec2&, const Vec2&)
 */
Vec2 operator-(const Vec2& a, const Vec2& b) {
    return {a.x - b.x, a.y - b.y};
}

/**
 * @copydoc operator*(const Vec2&, double)
 */
Vec2 operator*(const Vec2& a, double s) {
    return {a.x * s, a.y * s};
}

/**
 * @copydoc dot(const Vec2&, const Vec2&)
 */
double dot(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
}

/**
 * @copydoc cross(const Vec2&, const Vec2&)
 */
double cross(const Vec2& a, const Vec2& b) {
    return a.x * b.y - a.y * b.x;
}

/**
 * @copydoc triangle_twice_area(const Vec2&, const Vec2&, const Vec2&)
 */
double triangle_twice_area(const Vec2& a, const Vec2& b, const Vec2& c) {
    return cross(b - a, c - a);
}

/**
 * @copydoc norm(const Vec2&)
 */
double norm(const Vec2& a) {
    return std::sqrt(dot(a, a));
}

/**
 * @copydoc sign_with_eps(double)
 */
int sign_with_eps(double value) {
    if (value > kEps) {
        return 1;
    }
    if (value < -kEps) {
        return -1;
    }
    return 0;
}

/**
 * @copydoc signed_area_of_ring_points(const std::vector<Vec2>&)
 */
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

/**
 * @copydoc on_segment(const Vec2&, const Vec2&, const Vec2&)
 */
bool on_segment(const Vec2& a, const Vec2& b, const Vec2& p) {
    return std::min(a.x, b.x) - kEps <= p.x && p.x <= std::max(a.x, b.x) + kEps &&
           std::min(a.y, b.y) - kEps <= p.y && p.y <= std::max(a.y, b.y) + kEps;
}

/**
 * @copydoc segments_intersect(const Vec2&, const Vec2&, const Vec2&, const Vec2&)
 */
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

/**
 * @copydoc point_on_ring_boundary(const std::vector<Vec2>&, const Vec2&)
 */
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

/**
 * @copydoc point_in_ring_strict(const std::vector<Vec2>&, const Vec2&)
 */
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

/**
 * @copydoc same_ring_segments_are_adjacent(std::size_t, std::size_t, std::size_t)
 */
bool same_ring_segments_are_adjacent(std::size_t n, std::size_t i, std::size_t j) {
    if (i == j) {
        return true;
    }
    if ((i + 1) % n == j || (j + 1) % n == i) {
        return true;
    }
    return false;
}

}  // namespace polygon_simplification
