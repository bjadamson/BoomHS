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

template <template <class, std::size_t> typename C, std::size_t NUM_VERTEXES,
         std::size_t NUM_ELEMENTS, std::size_t NUM_OF_F_PER_VERTEX>
class shape_data
{
  using FloatType = float; // TODO: project-wide configuration (double precision maybe)
  using ElementType = GLuint;
  using VertexContainer = C<FloatType, NUM_VERTEXES>;
  using ElementContainer = C<ElementType, NUM_ELEMENTS>;

  GLenum const mode_;
  VertexContainer data_;
  ElementContainer elements_;

  NO_COPY(shape_data);
public:
  MOVE_DEFAULT(shape_data);
  explicit constexpr shape_data(GLenum const m, VertexContainer &&d, ElementContainer const& e)
    : mode_(m)
    , data_(std::move(d))
    , elements_(e)
  {
  }
  constexpr auto draw_mode() const { return this->mode_; }
  constexpr decltype(auto) vertices_data() const { return this->data_.data(); }
  constexpr auto vertices_length() const { return this->data_.size(); }
  constexpr std::size_t vertices_size_in_bytes() const { return this->vertices_length() * sizeof(FloatType); }
  constexpr auto vertice_count() const { return this->vertices_length() / NUM_OF_F_PER_VERTEX; }

  constexpr decltype(auto) elements_data() const { return this->elements_.data(); }
  constexpr std::size_t elements_count() const { return this->elements_.size(); }
  constexpr auto elements_size_in_bytes() const { return this->elements_count() * sizeof(ElementType); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// static data
template <std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS, std::size_t NUM_OF_F_PER_VERTEX>
using static_shape_array = shape_data<std::array, NUM_VERTEXES, NUM_ELEMENTS, NUM_OF_F_PER_VERTEX>;

template<std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS>
using static_vertex_color_shape = static_shape_array<NUM_VERTEXES, NUM_ELEMENTS, 8>;

template<std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS>
using static_vertex_uv_shape = static_shape_array<NUM_VERTEXES, NUM_ELEMENTS, 6>;

template<std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS>
using static_vertex_only_shape = static_shape_array<NUM_VERTEXES, NUM_ELEMENTS, 4>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// runtime-driven data
template <template <class, class> typename C, std::size_t NUM_OF_F_PER_VERTEX,
         typename F = float, typename A = std::allocator<F>, typename B = std::allocator<GLuint>>
class runtime_shape_template
{
  GLenum const mode_;
  C<F, A> data_;
  C<GLuint, B> elements_;

  NO_COPY(runtime_shape_template);
public:
  MOVE_DEFAULT(runtime_shape_template);
  explicit runtime_shape_template(GLenum const m, C<F, A> &&d, C<GLuint, B> &&e)
      : mode_(m)
      , data_(std::move(d))
      , elements_(std::move(e))
  {
  }
  auto draw_mode() const { return this->mode_; }
  auto vertices_data() const { return this->data_.data(); }

  std::size_t vertices_size_in_bytes() const { return this->vertices_length() * sizeof(F); }
  auto vertice_count() const { return this->vertices_length() / NUM_OF_F_PER_VERTEX; }
  auto vertices_length() const { return this->data_.length(); }

  decltype(auto) elements_data() const { return this->elements_.data(); }
  std::size_t elements_count() const { return this->elements_.length(); }
  auto elements_size_in_bytes() const { return this->elements_count() * sizeof(GLuint); }
};

template <std::size_t NUM_VERTEXES>
using runtime_sized_array = runtime_shape_template<stlw::sized_buffer, NUM_VERTEXES>;

// float triangles
using float_vertex_color_triangle = static_vertex_color_shape<24, 3>;
using float_vertex_uv_triangle = static_vertex_uv_shape<18, 3>;
using float_vertex_only_triangle = static_vertex_only_shape<12, 3>;

// float rectangles
using float_vertex_color_rectangle = static_vertex_color_shape<32, 6>;
using float_vertex_uv_rectangle = static_vertex_uv_shape<24, 6>;
using float_vertex_only_rectangle = static_vertex_only_shape<16, 6>;

// float cubes
using float_vertex_color_cube = static_vertex_color_shape<64, 36>;

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
  calc_vertex_uv_num_floats(GLint const num_v)
  {
    return (num_v * 4) + (num_v * 2);
  }

  static constexpr auto
  calc_vertex_only_num_floats(GLint const num_v)
  {
    return num_v * 4;
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
    auto constexpr elements = std::array<GLuint, 3>{0, 1, 2};
    return float_vertex_color_triangle{GL_TRIANGLES, std::move(floats), elements};
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
    auto constexpr elements = std::array<GLuint, 3>{0, 1, 2};
    return float_vertex_uv_triangle{GL_TRIANGLES, std::move(floats), elements};
  }

  static constexpr auto map_to_array_floats(game::triangle<game::wireframe_attributes> const &t)
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
    auto constexpr elements = std::array<GLuint, 3>{0, 1, 2};
    return float_vertex_only_triangle{GL_LINE_LOOP, std::move(floats), elements};
  }

///////////////////////////////////////////////////////////////////////////////////////////////////
// rectangles
  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_color_attributes> const &r)
  {
    using X = game::rectangle<game::vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
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

        r.top_left.vertex.x,     r.top_left.vertex.y,     r.top_left.vertex.z, // vertice 4
        r.top_left.vertex.w,     r.top_left.color.r,      r.top_left.color.g,
        r.top_left.color.b,      r.top_left.color.a
    };
    auto constexpr elements = std::array<GLuint, 6>{0, 1, 2, 2, 3, 0};
    return float_vertex_color_rectangle{GL_TRIANGLE_STRIP, std::move(floats), elements};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_uv_attributes> const &r)
  {
    using X = game::rectangle<game::vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_uv_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,  r.bottom_left.uv.u,   r.bottom_left.uv.v,

        r.bottom_right.vertex.x, r.bottom_right.vertex.y, r.bottom_right.vertex.z, // vertice 2
        r.bottom_right.vertex.w, r.bottom_right.uv.u,  r.bottom_right.uv.v,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,    r.top_right.uv.u,     r.top_right.uv.v,

        r.top_left.vertex.x,     r.top_left.vertex.y,     r.top_left.vertex.z, // vertice 4
        r.top_left.vertex.w,     r.top_left.uv.u,      r.top_left.uv.v,
    };
    auto constexpr elements = std::array<GLuint, 6>{0, 1, 2, 2, 3, 0};
    return float_vertex_uv_rectangle{GL_TRIANGLE_STRIP, std::move(floats), elements};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::wireframe_attributes> const &r)
  {
    using X = game::rectangle<game::wireframe_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);

    auto floats = std::array<float, NUM_FLOATS>{
        r.bottom_left.vertex.x,  r.bottom_left.vertex.y,  r.bottom_left.vertex.z, // vertice 1
        r.bottom_left.vertex.w,

        r.bottom_right.vertex.x, r.bottom_right.vertex.y, r.bottom_right.vertex.z, // vertice 2
        r.bottom_right.vertex.w,

        r.top_right.vertex.x,    r.top_right.vertex.y,    r.top_right.vertex.z, // vertice 3
        r.top_right.vertex.w,

        r.top_left.vertex.x,     r.top_left.vertex.y,     r.top_left.vertex.z, // vertice 4
        r.top_left.vertex.w,
    };
    auto constexpr elements = std::array<GLuint, 6>{0, 1, 2, 2, 3, 0};
    return float_vertex_only_rectangle{GL_LINE_LOOP, std::move(floats), elements};
  }

///////////////////////////////////////////////////////////////////////////////////////////////////
// cubes
  static constexpr auto map_to_array_floats(game::cube<game::vertex_color_attributes> const &r)
  {
    using X = game::cube<game::vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);

    auto const& v = r.vertices;
    auto const& c = r.vertices[0].color;

    auto floats = std::array<float, NUM_FLOATS>{
        v[0].vertex.x, v[0].vertex.y, v[0].vertex.z, v[0].vertex.w, // vertice 0
        v[0].color.r,  v[0].color.g,  v[0].color.b,  v[0].color.a,

        v[1].vertex.x, v[1].vertex.y, v[1].vertex.z, v[1].vertex.w, // vertice 1
        v[1].color.r,  v[1].color.g,  v[1].color.b,  v[1].color.a,

        v[2].vertex.x, v[2].vertex.y, v[2].vertex.z, v[2].vertex.w, // vertice 2
        v[2].color.r,  v[2].color.g,  v[2].color.b,  v[2].color.a,

        v[3].vertex.x, v[3].vertex.y, v[3].vertex.z, v[3].vertex.w, // vertice 3
        v[3].color.r,  v[3].color.g,  v[3].color.b,  v[3].color.a,

        v[4].vertex.x, v[4].vertex.y, v[4].vertex.z, v[4].vertex.w, // vertice 4
        v[4].color.r,  v[4].color.g,  v[4].color.b,  v[4].color.a,

        v[5].vertex.x, v[5].vertex.y, v[5].vertex.z, v[5].vertex.w, // vertice 5
        v[5].color.r,  v[5].color.g,  v[5].color.b,  v[5].color.a,

        v[6].vertex.x, v[6].vertex.y, v[6].vertex.z, v[6].vertex.w, // vertice 6
        v[6].color.r,  v[6].color.g,  v[6].color.b,  v[6].color.a,

        v[7].vertex.x, v[7].vertex.y, v[7].vertex.z, v[7].vertex.w, // vertice 7
        v[7].color.r,  v[7].color.g,  v[7].color.b,  v[7].color.a,
    };
    auto constexpr elements = std::array<GLuint, 36>{
      0, 1, 2, // front
      2, 3, 0,

      0, 7, 3, // left-side
      3, 6, 7,

      7, 4, 0, // floor
      3, 6, 4,

      4, 7, 6, // back
      6, 4, 5,

      5, 4, 1, // right-side
      1, 5, 2,

      2, 3, 6, // top
      6, 5, 2,
    };
    return float_vertex_color_cube{GL_TRIANGLE_STRIP, std::move(floats), elements};
  }

//////////////////////////////////////////////////////////////////////////////////////////////////
// polygon
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
    auto const elements_length = num_vertices;
    stlw::sized_buffer<GLuint> elements{static_cast<size_t>(elements_length)};
    for (auto i{0}; i < elements_length; ++i) {
      elements[i] = i;
    }
    return float_vertex_color_polygon{GL_TRIANGLE_FAN, std::move(floats), std::move(elements)};
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
    auto const elements_length = num_vertices;
    stlw::sized_buffer<GLuint> elements{static_cast<size_t>(elements_length)};
    for (auto i{0}; i < elements_length; ++i) {
      elements[i] = i;
    }
    return float_vertex_uv_polygon{GL_TRIANGLE_FAN, std::move(floats), std::move(elements)};
  }

public:
  template <typename... T, typename... R>
  static constexpr auto map_to_floats(T &&... shapes)
  {
    return std::make_tuple(shape_mapper::map_to_array_floats(shapes)...);
  }
};

} // ns engine::gfx::opengl
