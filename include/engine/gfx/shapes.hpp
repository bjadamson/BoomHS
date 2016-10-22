#pragma once
#include <array>
#include <tuple>
#include <game/data_types.hpp>

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

  static constexpr std::array<float, 4> DEFAULT_TEXTURE() { return {1.0f, 0.0f, 0.0f, 0.0f}; };
};
struct texture_coord {
  float const u, v;
  explicit constexpr texture_coord(float const up, float const vp)
      : u(up)
      , v(vp)
  {
  }
};
template<int N>
struct shape {
  std::array<point, N> const points;
  std::array<color, N> const colors;
  std::array<texture_coord, N> const coords;

  explicit constexpr shape(std::array<point, N> const &points_p,
                              std::array<color, N> const &colors_p,
                              std::array<texture_coord, N> const &coords_p)
      : points(points_p)
      , colors(colors_p)
      , coords(coords_p)
  {
  }
};

using triangle = shape<3>;
using rectangle = shape<4>;

// TYPE CONSTRUCTORS
triangle constexpr make_triangle(std::array<float, 12> const &p, std::array<float, 12> const &c,
                                 std::array<float, 6> const &tcoords)
{
  point const p0{p[0], p[1], p[2], p[3]};
  point const p1{p[4], p[5], p[6], p[7]};
  point const p2{p[8], p[9], p[10], p[11]};

  color const c0{c[0], c[1], c[2], c[3]};
  color const c1{c[4], c[5], c[6], c[7]};
  color const c2{c[8], c[9], c[10], c[11]};

  texture_coord const t0{tcoords[0], tcoords[1]};
  texture_coord const t1{tcoords[2], tcoords[3]};
  texture_coord const t2{tcoords[4], tcoords[5]};

  std::array<point, 3> const points{p0, p1, p2};
  std::array<color, 3> const colors{c0, c1, c2};
  std::array<texture_coord, 3> const texture_coordinates{t0, t1, t2};
  return triangle{points, colors, texture_coordinates};
}

triangle constexpr make_triangle(std::array<float, 12> const &p, std::array<float, 12> const &c)
{
  // clang-format off
  std::array<float, 6> const texture_coords = {
    0.0f, 0.0f, // lower-left corner
    1.0f, 0.0f, // lower-right corner
    0.5f, 1.0f  // top-middle corner
  };
  // clang-format on
  return make_triangle(p, c, texture_coords);
}

// type constructor for
// The color argument is assumed for all points in the triangle.
triangle constexpr make_triangle(std::array<float, 12> const &p, std::array<float, 4> const &c)
{
  // clang-format off
  std::array<float, 12> const colors = {
      c[0], c[1], c[2], c[3],
      c[0], c[1], c[2], c[3],
      c[0], c[1], c[2], c[3]
  };
  // clang-format on
  return make_triangle(p, colors);
}

auto constexpr make_triangle(std::array<float, 12> const &p)
{
  return make_triangle(p, color::DEFAULT_TEXTURE());
}

auto constexpr make_rectangle(std::array<float, 16> const &p, std::array<float, 16> const &c,
                                 std::array<float, 8> const &tcoords)
{
  point const p0{p[0], p[1], p[2], p[3]};
  point const p1{p[4], p[5], p[6], p[7]};
  point const p2{p[8], p[9], p[10], p[11]};
  point const p3{p[12], p[13], p[14], p[15]};

  color const c0{c[0], c[1], c[2], c[3]};
  color const c1{c[4], c[5], c[6], c[7]};
  color const c2{c[8], c[9], c[10], c[11]};
  color const c3{c[12], c[13], c[14], c[15]};

  texture_coord const t0{tcoords[0], tcoords[1]};
  texture_coord const t1{tcoords[2], tcoords[3]};
  texture_coord const t2{tcoords[4], tcoords[5]};
  texture_coord const t3{tcoords[6], tcoords[7]};

  std::array<point, 4> const points{p0, p1, p2, p3};
  std::array<color, 4> const colors{c0, c1, c2, c3};
  std::array<texture_coord, 4> const texture_coordinates{t0, t1, t2, t3};
  return rectangle{points, colors, texture_coordinates};
}

auto constexpr make_rectangle(std::array<float, 16> const &p, std::array<float, 16> const &c)
{
  // clang-format off
  std::array<float, 8> const texture_coords = {
    0.0f, 0.0f, // lower-left
    1.0f, 0.0f, // lower-right
    1.0f, 1.0f, // top-right
    0.0f, 1.0f  // top-left
  };
  // clang-format on
  return make_rectangle(p, c, texture_coords);
}

auto constexpr make_rectangle(std::array<float, 16> const &p, std::array<float, 4> const &c)
{
  std::array<float, 16> const colors = {
      c[0], c[1], c[2], c[3],
      c[0], c[1], c[2], c[3],
      c[0], c[1], c[2], c[3],
      c[0], c[1], c[2], c[3]
  };
  return make_rectangle(p, colors);
}

auto constexpr make_rectangle(std::array<float, 16> const &p)
{
  return make_rectangle(p, color::DEFAULT_TEXTURE());
}

} // ns gfx
} // ns engine
