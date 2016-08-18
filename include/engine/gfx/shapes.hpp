#pragma once
#include <array>
#include <tuple>

namespace engine
{
namespace gfx
{
struct point {
  float const x, y, z, w;
  explicit constexpr point(float const xp, float const yp, float const zp, float const wp)
      : x(xp)
      , y(yp)
      , z(zp)
      , w(wp)
  {
  }
};
struct color {
  float const r, g, b, a;
  explicit constexpr color(float const rp, float const gp, float const bp, float const ap)
      : r(rp)
      , g(gp)
      , b(bp)
      , a(ap)
  {
  }
};
struct triangle {
  std::array<point, 3> const points;
  std::array<color, 3> const colors;
  explicit constexpr triangle(std::array<point, 3> const &p, std::array<color, 3> const &c)
      : points(p)
      , colors(c)
  {
  }
};

triangle constexpr make_triangle(std::array<float, 12> const &p, std::array<float, 12> const &c)
{
  point const p0{p[0], p[1], p[2], p[3]};
  point const p1{p[4], p[5], p[6], p[7]};
  point const p2{p[8], p[9], p[10], p[11]};

  color const c0{c[0], c[1], c[2], c[3]};
  color const c1{c[4], c[5], c[6], c[7]};
  color const c2{c[8], c[9], c[10], c[11]};

  std::array<point, 3> const points{p0, p1, p2};
  std::array<color, 3> const colors{c0, c1, c2};
  return triangle{points, colors};
}

// type constructor for
// The color argument is assumed for all points in the triangle.
triangle constexpr make_triangle(std::array<float, 12> const &p, std::array<float, 4> const &c)
{
  // clang-format off
    std::array<float, 12> const colors{
      c[0], c[1], c[2], c[3],
      c[0], c[1], c[2], c[3],
      c[0], c[1], c[2], c[3]};
  // clang-format on
  return make_triangle(p, colors);
}

} // ns gfx
} // ns engine
