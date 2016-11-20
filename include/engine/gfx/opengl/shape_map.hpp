#pragma once
#include <array>
#include <cassert>
#include <tuple>
#include <stlw/tuple.hpp>

#include <engine/gfx/shapes.hpp>

namespace engine::gfx::opengl
{

constexpr auto
to_triangle(game::triangle const& t)
{
  auto const wc = t.wc();
  using namespace engine::gfx;
  constexpr float radius = 0.5;

  // clang-format off
  std::array<float, 12> const vertices =
  {
    t.bottom_left.x,  t.bottom_left.y,  t.bottom_left.z,  t.bottom_left.w,
    t.bottom_right.x, t.bottom_right.y, t.bottom_right.z, t.bottom_right.w,
    t.top_middle.x,   t.top_middle.y,   t.top_middle.z,   t.top_middle.w
  };
  constexpr std::array<float, 12> const colors =
  {
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
  };
  return ::engine::gfx::make_triangle(vertices, colors);
  // clang-format on
}

constexpr auto
to_rectangle(game::rectangle const& r)
{
  auto const wc = r.wc();
  using namespace engine::gfx;
  constexpr float width = 0.25;
  constexpr float height = 0.39;

  // clang-format off
  std::array<float, 16> const vertices =
  {
    r.bottom_left.x,  r.bottom_left.y,  r.bottom_left.z,  r.bottom_left.w,
    r.bottom_right.x, r.bottom_right.y, r.bottom_right.z, r.bottom_right.w,
    r.top_right.x,    r.top_right.y,    r.top_right.z,    r.top_right.w,
    r.top_left.x,     r.top_left.y,     r.top_left.z,     r.top_left.w
  };
  constexpr std::array<float, 16> colors =
  {
    1.0f, 0.0f, 0.0f, 1.0f, // r
    0.0f, 1.0f, 0.0f, 1.0f, // g
    0.0f, 0.0f, 1.0f, 1.0f, // b
    1.0f, 1.0f, 0.2f, 1.0f  // a
  };
  return ::engine::gfx::make_rectangle(vertices, colors);
  // clang-format on
}

auto
to_polygon(game::polygon const& p)
{
  auto const num_floats = p.num_vertices() * 4; // TODO: don't hardcode this assumption.
  stlw::sized_buffer<float> vertices{num_floats};
  for (auto i{0}; i < num_floats;) {
    vertices[i++] = 1.0f; // r
    vertices[i++] = 0.0f; // g
    vertices[i++] = 0.0f; // b
    vertices[i++] = 1.0f; // a
  }

  stlw::sized_buffer<float> colors{num_floats};
  for (auto i{0}; i < num_floats;) {
    colors[i++] = 1.0f; // r
    colors[i++] = 0.0f; // g
    colors[i++] = 0.0f; // b
    colors[i++] = 1.0f; // a
  }
  return ::engine::gfx::make_polygon(vertices, colors);
}

// For now we assume 10 attributes per-vertex:
// 4 vertex (x,y,z,w)
// 4 colors (r,g,b,a),
// 2 tex coords (u, v)
template <int N>
class static_shape
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
  vertices[INDEX].x, vertices[INDEX].y, vertices[INDEX].z, vertices[INDEX].w, /* point */          \
  colors[INDEX].r,   colors[INDEX].g,   colors[INDEX].b,   colors[INDEX].a,   /* color */          \
  tcords[INDEX].u,   tcords[INDEX].v                                          /* texture coords */
// clang-format on

using triangle = static_shape<30>;
auto constexpr map_to_gl_priv(::engine::gfx::triangle const &gfx_triangle)
{
  auto const &vertices = gfx_triangle.vertices;
  auto const &colors = gfx_triangle.colors;
  auto const &tcords = gfx_triangle.coords;

  return triangle{GL_TRIANGLES, {ROW(0), ROW(1), ROW(2)}};
}

using rectangle = static_shape<40>;
auto constexpr map_to_gl_priv(::engine::gfx::rectangle const &gfx_rect)
{
  auto const &vertices = gfx_rect.vertices;
  auto const &colors = gfx_rect.colors;
  auto const &tcords = gfx_rect.coords;

  return rectangle{GL_QUADS, {ROW(0), ROW(1), ROW(2), ROW(3)}};
}

triangle constexpr map_to_gl(game::triangle const& triangle)
{
  return map_to_gl_priv(to_triangle(triangle));
}

rectangle constexpr map_to_gl(game::rectangle const& rect)
{
  return map_to_gl_priv(to_rectangle(rect));
}

template<typename ...T, typename ...R>
auto constexpr map_to_gl(T &&... shapes)
{
  return std::make_tuple(map_to_gl(shapes)...);
}

} // ns engine::gfx::opengl
