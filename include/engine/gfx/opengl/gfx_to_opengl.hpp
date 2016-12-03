#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/glew.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/scene_renderer.hpp>
#include <engine/gfx/opengl/vertex_attribute.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/shape.hpp>
#include <glm/glm.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_ctors.hpp>

namespace engine::gfx::opengl
{

template <template <class, std::size_t> typename C, std::size_t V, std::size_t NUM_OF_F_PER_VERTEX>
class static_shape_template
{
  using F = float;
  GLenum const mode_;
  C<F, V> data_;

  NO_COPY(static_shape_template);
public:
  MOVE_DEFAULT(static_shape_template);
  explicit constexpr static_shape_template(GLenum const m, C<F, V> &&d)
      : mode_(m)
      , data_(std::move(d))
  {
  }
  constexpr auto draw_mode() const { return this->mode_; }
  constexpr auto data() const { return this->data_.data(); }

  constexpr std::size_t size_in_bytes() const { return this->length() * sizeof(F); }
  constexpr auto vertice_count() const { return this->length() / NUM_OF_F_PER_VERTEX; }
  constexpr auto length() const { return this->data_.size(); }
};

template <std::size_t V, std::size_t NUM_OF_F_PER_VERTEX>
using static_shape_array = static_shape_template<std::array, V, NUM_OF_F_PER_VERTEX>;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
template<std::size_t V>
using static_vertex_color_shape = static_shape_array<V, 8>;

template<std::size_t V>
using static_vertex_uv_shape = static_shape_array<V, 6>;

template<std::size_t V>
using static_vertex_only_shape = static_shape_array<V, 4>;

template <template <class, class> typename C, std::size_t NUM_OF_F_PER_VERTEX,
         typename F = float, typename A = std::allocator<F>>
class runtime_shape_template
{
  GLenum const mode_;
  C<F, A> data_;

  NO_COPY(runtime_shape_template);
public:
  MOVE_DEFAULT(runtime_shape_template);
  explicit runtime_shape_template(GLenum const m, C<F, A> &&d)
      : mode_(m)
      , data_(std::move(d))
  {
  }
  auto draw_mode() const { return this->mode_; }
  auto data() const { return this->data_.data(); }

  std::size_t size_in_bytes() const { return this->length() * sizeof(F); }
  auto vertice_count() const { return this->length() / NUM_OF_F_PER_VERTEX; }
  auto length() const { return this->data_.length(); }
};

template <std::size_t V>
using runtime_sized_array = runtime_shape_template<stlw::sized_buffer, V>;

// float triangles
using float_vertex_color_triangle = static_vertex_color_shape<24>;
using float_vertex_uv_triangle = static_vertex_uv_shape<18>;
using float_vertex_only_triangle = static_vertex_only_shape<12>;

// float rectangles
using float_vertex_color_rectangle = static_vertex_color_shape<48>;
using float_vertex_uv_rectangle = static_vertex_uv_shape<36>;
using float_vertex_only_rectangle = static_vertex_only_shape<24>;

// float polygons
using float_vertex_color_polygon = runtime_sized_array<8>;
using float_vertex_uv_polygon = runtime_sized_array<6>;
using float_vertex_only_polygon = runtime_sized_array<4>;

class shape_mapper
{
  shape_mapper() = delete;

  static constexpr auto
  calc_vertex_color_num_floats(GLint const num_v)
  {
    return (num_v * 4) + (num_v * 4);
  }

  static constexpr auto
  calc_vertex_only_num_floats(GLint const num_v)
  {
    return num_v * 4;
  }

  static constexpr auto
  calc_vertex_uv_num_floats(GLint const num_v)
  {
    return (num_v * 4) + (num_v * 2);
  }

  static constexpr auto map_to_array_floats(game::triangle<game::vertex_color_attributes> const &t)
  {
    using X = game::triangle<game::vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        t.bottom_left.vertex.x,  t.bottom_left.vertex.y,  t.bottom_left.vertex.z,
        t.bottom_left.vertex.w,  t.bottom_left.color.r,   t.bottom_left.color.g,
        t.bottom_left.color.b,   t.bottom_left.color.a,

        t.bottom_right.vertex.x, t.bottom_right.vertex.y, t.bottom_right.vertex.z,
        t.bottom_right.vertex.w, t.bottom_right.color.r,  t.bottom_right.color.g,
        t.bottom_right.color.b,  t.bottom_right.color.a,

        t.top_middle.vertex.x,   t.top_middle.vertex.y,   t.top_middle.vertex.z,
        t.top_middle.vertex.w,   t.top_middle.color.r,    t.top_middle.color.g,
        t.top_middle.color.b,    t.top_middle.color.a
    };
    return float_vertex_color_triangle{GL_TRIANGLES, std::move(floats)};
  }

  static constexpr auto map_to_array_floats(game::triangle<game::vertex_uv_attributes> const &t)
  {
    using X = game::triangle<game::vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_uv_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        t.bottom_left.vertex.x,  t.bottom_left.vertex.y, t.bottom_left.vertex.z,
        t.bottom_left.vertex.w,  t.bottom_left.uv.u,     t.bottom_left.uv.v,

        t.bottom_right.vertex.x, t.bottom_right.vertex.y, t.bottom_right.vertex.z,
        t.bottom_right.vertex.w, t.bottom_right.uv.u,     t.bottom_right.uv.v,

        t.top_middle.vertex.x,   t.top_middle.vertex.y, t.top_middle.vertex.z,
        t.top_middle.vertex.w,   t.top_middle.uv.u,     t.top_middle.uv.v,
    };
    return float_vertex_uv_triangle{GL_TRIANGLES, std::move(floats)};
  }

  static constexpr auto map_to_array_floats(game::triangle<game::vertex_wireframe_attributes> const &t)
  {
    using X = game::triangle<game::vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        t.bottom_left.vertex.x,  t.bottom_left.vertex.y,  t.bottom_left.vertex.z,
        t.bottom_left.vertex.w,

        t.bottom_right.vertex.x, t.bottom_right.vertex.y, t.bottom_right.vertex.z,
        t.bottom_right.vertex.w,

        t.top_middle.vertex.x,   t.top_middle.vertex.y,   t.top_middle.vertex.z,
        t.top_middle.vertex.w
    };
    return float_vertex_only_triangle{GL_LINE_LOOP, std::move(floats)};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_color_attributes> const &r)
  {
    auto constexpr NUM_VERTICES = 6;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,  r.bottom_left.color.r,   r.bottom_left.color.g,
        r.bottom_left.color.b,   r.bottom_left.color.a,

        r.bottom_right.vertex.x, r.bottom_right.vertex.y, r.bottom_right.vertex.z, // vertice 2
        r.bottom_right.vertex.w, r.bottom_right.color.r,  r.bottom_right.color.g,
        r.bottom_right.color.b,  r.bottom_right.color.a,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,    r.top_right.color.r,     r.top_right.color.g,
        r.top_right.color.b,     r.top_right.color.a,

        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,  r.bottom_left.color.r,   r.bottom_left.color.g,
        r.bottom_left.color.b,   r.bottom_left.color.a,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,    r.top_right.color.r,     r.top_right.color.g,
        r.top_right.color.b,     r.top_right.color.a,

        r.top_left.vertex.x,     r.top_left.vertex.y,     r.top_left.vertex.z, // vertice 4
        r.top_left.vertex.w,     r.top_left.color.r,      r.top_left.color.g,
        r.top_left.color.b,      r.top_left.color.a,
    };
    return float_vertex_color_rectangle{GL_TRIANGLE_STRIP, std::move(floats)};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_uv_attributes> const &r)
  {
    auto constexpr NUM_VERTICES = 6;
    auto constexpr NUM_FLOATS = calc_vertex_uv_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,  r.bottom_left.uv.u,   r.bottom_left.uv.v,

        r.bottom_right.vertex.x, r.bottom_right.vertex.y, r.bottom_right.vertex.z, // vertice 2
        r.bottom_right.vertex.w, r.bottom_right.uv.u,  r.bottom_right.uv.v,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,    r.top_right.uv.u,     r.top_right.uv.v,

        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,  r.bottom_left.uv.u,   r.bottom_left.uv.v,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,    r.top_right.uv.u,     r.top_right.uv.v,

        r.top_left.vertex.x,     r.top_left.vertex.y,     r.top_left.vertex.z, // vertice 4
        r.top_left.vertex.w,     r.top_left.uv.u,      r.top_left.uv.v,
    };
    return float_vertex_uv_rectangle{GL_TRIANGLE_STRIP, std::move(floats)};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_wireframe_attributes> const &r)
  {
    auto constexpr NUM_VERTICES = 6;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,

        r.bottom_right.vertex.x, r.bottom_right.vertex.y, r.bottom_right.vertex.z, // vertice 2
        r.bottom_right.vertex.w,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,

        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,

        r.top_left.vertex.x,     r.top_left.vertex.y,     r.top_left.vertex.z, // vertice 4
        r.top_left.vertex.w,
    };
    return float_vertex_only_rectangle{GL_LINE_LOOP, std::move(floats)};
  }

  static auto map_to_array_floats(game::polygon<game::vertex_color_attributes> const &p)
  {
    auto const num_vertices = p.num_vertices();
    auto const num_floats = calc_vertex_color_num_floats(num_vertices);

    stlw::sized_buffer<float> floats{static_cast<size_t>(num_floats)};
    for (auto i{0}, j{0}; j < floats.length(); ++i) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertice.vertex.x;
      floats[j++] = vertice.vertex.y;
      floats[j++] = vertice.vertex.z;
      floats[j++] = vertice.vertex.w;

      floats[j++] = vertice.color.r;
      floats[j++] = vertice.color.g;
      floats[j++] = vertice.color.b;
      floats[j++] = vertice.color.a;
    }
    // TODO: GL_POLYGON is deprecated
    return float_vertex_color_polygon{GL_POLYGON, std::move(floats)};
  }

  static auto map_to_array_floats(game::polygon<game::vertex_uv_attributes> const &p)
  {
    auto const num_vertices = p.num_vertices();
    auto const num_floats = calc_vertex_uv_num_floats(num_vertices);

    stlw::sized_buffer<float> floats{static_cast<size_t>(num_floats)};
    for (auto i{0}, j{0}; j < floats.length(); ++i) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertice.vertex.x;
      floats[j++] = vertice.vertex.y;
      floats[j++] = vertice.vertex.z;
      floats[j++] = vertice.vertex.w;

      floats[j++] = vertice.uv.u;
      floats[j++] = vertice.uv.v;
    }
    // TODO: GL_POLYGON is deprecated
    return float_vertex_uv_polygon{GL_POLYGON, std::move(floats)};
  }

public:
  template <typename... T, typename... R>
  static constexpr auto map_to_floats(T &&... shapes)
  {
    return std::make_tuple(shape_mapper::map_to_array_floats(shapes)...);
  }
};

} // ns engine::gfx::opengl
