#pragma once
#include <array>
#include <cassert>
#include <tuple>
#include <stlw/tuple.hpp>

#include <engine/gfx/shapes.hpp>

namespace engine::gfx::opengl
{

constexpr auto
to_triangle(game::world_coordinate const& wc)
{
  using namespace engine::gfx;
  constexpr float radius = 0.5;

  // clang-format off
 std::array<float, 12> v0 =
  {
    wc.x() - radius, wc.y() - radius, wc.z(), wc.w(), // bottom left
    wc.x() + radius, wc.y() - radius, wc.z(), wc.w(), // bottom right
    wc.x()         , wc.y() + radius, wc.z(), wc.w()  // top middle
  };
  constexpr std::array<float, 12> c0 =
  {
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
  };
  return ::engine::gfx::make_triangle(v0, c0);
  // clang-format on
}

constexpr auto
to_rectangle(game::world_coordinate const& wc)
{
  using namespace engine::gfx;
  constexpr float width = 0.25;
  constexpr float height = 0.39;

  // clang-format off
 std::array<float, 16> v0 =
  {
    wc.x() - width, wc.y() - height, wc.z(), wc.w(), // bottom left
    wc.x() + width, wc.y() - height, wc.z(), wc.w(), // bottom right
    wc.x() + width, wc.y() + height, wc.z(), wc.w(), // top left
    wc.x() - width, wc.y() + height, wc.z(), wc.w() // top right
  };
  constexpr std::array<float, 16> c0 =
  {
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.2f, 1.0f,
  };
  return ::engine::gfx::make_rectangle(v0, c0);
  // clang-format on
}

// For now we assume 10 attributes per-vertex:
// 4 vertex (x,y,z,w)
// 4 colors (r,g,b,a),
// 2 tex coords (u, v)
template <int N>
class shape
{
  GLenum const mode_;
  std::array<float, N> const data_;
public:
  explicit constexpr shape(GLenum const m, std::array<float, N> const &d)
      : mode_(m)
      , data_(d)
  {
  }
  inline constexpr auto draw_mode() const { return this->mode_; }
  inline constexpr auto data() const { return this->data_.data(); }
  inline constexpr auto size_in_bytes() const { return N * sizeof(float); }
  inline constexpr auto vertice_count() const { return N/10; }
};

// clang-format off
#define ROW(INDEX)                                                                                 \
  vertices[INDEX].x, vertices[INDEX].y, vertices[INDEX].z, vertices[INDEX].w, /* point */                  \
  colors[INDEX].r, colors[INDEX].g, colors[INDEX].b, colors[INDEX].a, /* color */                  \
  tcords[INDEX].u, tcords[INDEX].v                                    /* texture coordinates */
// clang-format on

using triangle = ::engine::gfx::opengl::shape<30>;
auto constexpr map_to_gl_priv(::engine::gfx::triangle const &gfx_triangle)
{
  auto const &vertices = gfx_triangle.vertices;
  auto const &colors = gfx_triangle.colors;
  auto const &tcords = gfx_triangle.coords;

  return triangle{GL_TRIANGLES, {ROW(0), ROW(1), ROW(2)}};
}

using rectangle = ::engine::gfx::opengl::shape<40>;
auto constexpr map_to_gl_priv(::engine::gfx::rectangle const &gfx_rect)
{
  auto const &vertices = gfx_rect.vertices;
  auto const &colors = gfx_rect.colors;
  auto const &tcords = gfx_rect.coords;

  return rectangle{GL_QUADS, {ROW(0), ROW(1), ROW(2), ROW(3)}};
}

triangle constexpr map_to_gl(game::triangle const& sh)
{
  return map_to_gl_priv(to_triangle(sh.wc()));
}

rectangle constexpr map_to_gl(game::rectangle const& sh)
{
  return map_to_gl_priv(to_rectangle(sh.wc()));
}

template<typename ...T, typename ...R>
auto constexpr map_to_gl(T &&... shapes)
{
  return std::make_tuple(map_to_gl(shapes)...);
}

} // ns engine::gfx::opengl
