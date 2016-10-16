#pragma once
#include <array>
#include <engine/gfx/shapes.hpp>
#include <tuple>

namespace engine::gfx::opengl
{

// For now we assume 10 per-vertex because:
// 4 points (x,y,z,w)
// 4 colors (r,g,b,a),
// 2 tex coords (u, v)
template <int N>
class shape
{
  std::array<float, N> const data_;

public:
  explicit constexpr shape(std::array<float, N> const &d)
      : data_(d)
  {
  }
  inline constexpr decltype(auto) data() const { return this->data_.data(); }
  inline constexpr auto size_in_bytes() const { return N * sizeof(float); }
  inline constexpr auto vertice_count() const { return N/10; }
};

// clang-format off
#define ROW(INDEX)                                                                                 \
  points[INDEX].x, points[INDEX].y, points[INDEX].z, points[INDEX].w, /* point */                  \
  colors[INDEX].r, colors[INDEX].g, colors[INDEX].b, colors[INDEX].a, /* color */                  \
  tcords[INDEX].u, tcords[INDEX].v                                    /* texture coordinates */
// clang-format on

using triangle = ::engine::gfx::opengl::shape<30>;
auto constexpr map_to_gl(::engine::gfx::triangle const &gfx_triangle)
{
  auto const &points = gfx_triangle.points;
  auto const &colors = gfx_triangle.colors;
  auto const &tcords = gfx_triangle.coords;

  return triangle{{ROW(0), ROW(1), ROW(2)}};
}

using rectangle = ::engine::gfx::opengl::shape<40>;
auto constexpr map_to_gl(::engine::gfx::rectangle const &gfx_rect)
{
  auto const &points = gfx_rect.points;
  auto const &colors = gfx_rect.colors;
  auto const &tcords = gfx_rect.coords;

  return rectangle{{ROW(0), ROW(1), ROW(2), ROW(3)}};
}

} // ns engine::gfx::opengl
