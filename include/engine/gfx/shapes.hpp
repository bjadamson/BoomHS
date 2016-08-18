#pragma once
#include <array>

namespace engine
{
namespace gfx
{
struct point {
  float const x, y, z, w;
  explicit constexpr point(float const xp, float const yp, float const zp, float const wp) :
    x(xp),
    y(yp),
    z(zp),
    w(wp) {}
};
struct color {
  float const r, g, b, a;
  explicit constexpr color(float const rp, float const gp, float const bp, float const ap) :
    r(rp),
    g(gp),
    b(bp),
    a(ap) {}
};
struct triangle {
  std::array<point,3> const points;
  std::array<color,3> const colors;
  explicit constexpr triangle(std::array<point,3> const& p, std::array<color,3> const c) :
    points(p), colors(c)
  {
  }
};
struct rectangle {
  std::array<point, 4> const points;
  std::array<point, 4> const colors;
  explicit constexpr rectangle(std::array<point,4> const& p, std::array<color,4> const c) :
    points(p), colors(c)
  {
  }
};

} // ns gfx
} // ns engine
