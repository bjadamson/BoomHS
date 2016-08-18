#pragma once
#include <array>
#include <engine/gfx/shapes.hpp>
#include <tuple>

namespace engine
{
namespace gfx
{
namespace opengl
{
template <int N>
class shape
{
  std::array<float, N> const data_;

public:
  explicit constexpr shape(std::array<float, N> const &d)
      : data_(d)
  {
  }
  inline decltype(auto) data() const { return this->data_.data(); }
  inline auto size_in_bytes() const { return this->data_.size() * sizeof(float); }
};
using triangle = ::engine::gfx::opengl::shape<24>;

auto constexpr map_to_gl(::engine::gfx::triangle const &gfx_triangle)
{
  auto const &points = gfx_triangle.points;
  auto const &colors = gfx_triangle.colors;

  return triangle{{
      points[0].x, points[0].y, points[0].z, points[0].w, // point
      colors[0].r, colors[0].g, colors[0].b, colors[0].a, // color

      points[1].x, points[1].y, points[1].z, points[1].w, // point
      colors[1].r, colors[1].g, colors[1].b, colors[1].a, // color

      points[2].x, points[2].y, points[2].z, points[2].w, // point
      colors[2].r, colors[2].g, colors[2].b, colors[2].a  // color
  }};
}

} // ns opengl
} // ns gfx
} // ns engine
