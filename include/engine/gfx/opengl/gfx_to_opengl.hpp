#pragma once
#include <engine/gfx/mode.hpp>
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/glew.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/vertex_attribute.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/shape2d.hpp>
#include <game/shape3d.hpp>
#include <glm/glm.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_ctors.hpp>

namespace engine::gfx::opengl
{

// TODO: project-wide configuration (double precision maybe)
using FloatType = float;
using ElementType = GLuint;

template <template <class, std::size_t> typename C, std::size_t NUM_VERTEXES,
          std::size_t NUM_ELEMENTS, std::size_t NUM_OF_F_PER_VERTEX>
struct shape_data
{
  using VertexContainer = C<FloatType, NUM_VERTEXES>;
  using VertexOrdering = C<ElementType, NUM_ELEMENTS>;

  GLenum const draw_mode;
  VertexContainer vertices;
  VertexOrdering ordering;

  NO_COPY(shape_data);
  MOVE_DEFAULT(shape_data);
  explicit constexpr shape_data(GLenum const dm, VertexContainer &&v, VertexOrdering const &o)
      : draw_mode(dm)
      , vertices(std::move(v))
      , ordering(o)
  {
  }
  auto constexpr num_floats_per_vertex() const { return NUM_OF_F_PER_VERTEX; }
};

template <template <class, std::size_t> typename C, std::size_t NUM_VERTEXES,
          std::size_t NUM_ELEMENTS, std::size_t NUM_OF_F_PER_VERTEX>
struct shape3d_data : public shape_data<C, NUM_VERTEXES, NUM_ELEMENTS, NUM_OF_F_PER_VERTEX>
{
  using BASE = shape_data<C, NUM_VERTEXES, NUM_ELEMENTS, NUM_OF_F_PER_VERTEX>;
  using VertexContainer = typename BASE::VertexContainer;
  using VertexOrdering = typename BASE::VertexOrdering;

  game::model const& model;

  MOVE_DEFAULT(shape3d_data);
  NO_COPY(shape3d_data);
  explicit constexpr shape3d_data(GLenum const m, VertexContainer &&d, VertexOrdering const& e,
      game::model const& model)
    : BASE(m, std::move(d), e)
    , model(model)
  {
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// runtime-driven data
template <template <class, class> typename C, std::size_t NUM_OF_F_PER_VERTEX,
         typename A = std::allocator<FloatType>, typename B = std::allocator<ElementType>>
struct runtime_shape_template
{
  GLenum const draw_mode;
  C<FloatType, A> vertices;
  C<ElementType, B> ordering;

  NO_COPY(runtime_shape_template);
  MOVE_DEFAULT(runtime_shape_template);
  explicit runtime_shape_template(GLenum const dm, C<FloatType, A> &&d, C<ElementType, B> &&e)
      : draw_mode(dm)
      , vertices(std::move(d))
      , ordering(std::move(e))
  {
  }
  auto constexpr num_floats_per_vertex() const { return NUM_OF_F_PER_VERTEX; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// associated-functions
template <typename S>
constexpr auto
ordering_size_in_bytes(S const& s)
{
  return s.ordering.size() * sizeof(ElementType);
}

template <typename S>
constexpr auto
vertice_count(S const& s)
{
  return s.vertices.size() / s.num_floats_per_vertex();
}

template <typename S>
constexpr std::size_t
vertices_size_in_bytes(S const& s)
{
  return s.vertices.size() * sizeof(FloatType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// static templates
template <std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS, std::size_t NUM_OF_F_PER_VERTEX>
using static_shape_array = shape_data<std::array, NUM_VERTEXES, NUM_ELEMENTS, NUM_OF_F_PER_VERTEX>;

template <std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS, std::size_t NUM_OF_F_PER_VERTEX>
using static_shape_with_model_array = shape3d_data<std::array, NUM_VERTEXES, NUM_ELEMENTS,
      NUM_OF_F_PER_VERTEX>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// static shapes
template <std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS>
using static_vertex_color_shape = static_shape_array<NUM_VERTEXES, NUM_ELEMENTS, 8>;

template <std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS>
using static_vertex_uv_shape = static_shape_array<NUM_VERTEXES, NUM_ELEMENTS, 6>;

template <std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS>
using static_vertex_only_shape = static_shape_array<NUM_VERTEXES, NUM_ELEMENTS, 4>;

template <std::size_t NUM_VERTEXES, std::size_t NUM_ELEMENTS>
using static_vertex_model_shape = static_shape_with_model_array<NUM_VERTEXES, NUM_ELEMENTS, 4>;

// float triangles
using vertex_color_triangle = static_vertex_color_shape<24, 3>;
using vertex_uv_triangle = static_vertex_uv_shape<18, 3>;
using vertex_only_triangle = static_vertex_only_shape<12, 3>;

// float rectangles
using vertex_color_rectangle = static_vertex_color_shape<32, 6>;
using vertex_uv_rectangle = static_vertex_uv_shape<24, 6>;
using vertex_only_rectangle = static_vertex_only_shape<16, 6>;

// float cubes
using vertex_color_cube = static_vertex_model_shape<64, 14>;
using vertex_uv_cube = static_vertex_model_shape<32, 14>;
using vertex_only_cube = static_vertex_model_shape<32, 14>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// runtime-sized shapes
template <std::size_t NUM_VERTEXES>
using runtime_sized_array = runtime_shape_template<stlw::sized_buffer, NUM_VERTEXES>;

// float polygons
using vertex_color_polygon = runtime_sized_array<8>;
using vertex_uv_polygon = runtime_sized_array<7>;
using vertex_only_polygon = runtime_sized_array<4>;

class shape_mapper
{
  shape_mapper() = delete;

  static constexpr GLenum map_gfx_mode_to_opengl_mode(draw_mode const mode)
  {
    assert(mode <= draw_mode::INVALID_DRAW_MODE);

    switch (mode) {
      case draw_mode::TRIANGLES:
        return GL_TRIANGLES;
      case draw_mode::TRIANGLE_STRIP:
        return GL_TRIANGLE_STRIP;
      case draw_mode::TRIANGLE_FAN:
        return GL_TRIANGLE_FAN;
      case draw_mode::LINE_LOOP:
        return GL_LINE_LOOP;
      case draw_mode::INVALID_DRAW_MODE:
        // fall-through
        break;
    }

    // This shouldn't ever happen.
    assert(0 == 1);
  }

  static constexpr auto calc_vertex_color_num_floats(GLint const num_v)
  {
    return (num_v * 4) + (num_v * 4);
  }

  static constexpr auto calc_vertex_uv_num_floats(GLint const num_v)
  {
    return (num_v * 4) + (num_v * 2);
  }

  static constexpr auto calc_vertex_only_num_floats(GLint const num_v) { return num_v * 4; }

  static auto constexpr TRIANGLE_ELEMENTS() { return std::array<ElementType, 3>{0, 1, 2}; }
  static constexpr auto map_to_array_floats(game::triangle<game::vertex_color_attributes> const &t)
  {
    using X = game::triangle<game::vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        t.bottom_left.vertex.x,  t.bottom_left.vertex.y,  t.bottom_left.vertex.z,
        t.bottom_left.vertex.w,  t.bottom_left.color.r,   t.bottom_left.color.g,
        t.bottom_left.color.b,   t.bottom_left.color.a,

        t.bottom_right.vertex.x, t.bottom_right.vertex.y, t.bottom_right.vertex.z,
        t.bottom_right.vertex.w, t.bottom_right.color.r,  t.bottom_right.color.g,
        t.bottom_right.color.b,  t.bottom_right.color.a,

        t.top_middle.vertex.x,   t.top_middle.vertex.y,   t.top_middle.vertex.z,
        t.top_middle.vertex.w,   t.top_middle.color.r,    t.top_middle.color.g,
        t.top_middle.color.b,    t.top_middle.color.a};
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(t.draw_mode());
    return vertex_color_triangle{mode, std::move(floats), TRIANGLE_ELEMENTS()};
  }

  static constexpr auto map_to_array_floats(game::triangle<game::vertex_uv_attributes> const &t)
  {
    using X = game::triangle<game::vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_uv_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        t.bottom_left.vertex.x,  t.bottom_left.vertex.y,  t.bottom_left.vertex.z,
        t.bottom_left.vertex.w,  t.bottom_left.uv.u,      t.bottom_left.uv.v,

        t.bottom_right.vertex.x, t.bottom_right.vertex.y, t.bottom_right.vertex.z,
        t.bottom_right.vertex.w, t.bottom_right.uv.u,     t.bottom_right.uv.v,

        t.top_middle.vertex.x,   t.top_middle.vertex.y,   t.top_middle.vertex.z,
        t.top_middle.vertex.w,   t.top_middle.uv.u,       t.top_middle.uv.v};
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(t.draw_mode());
    return vertex_uv_triangle{mode, std::move(floats), TRIANGLE_ELEMENTS()};
  }

  static constexpr auto map_to_array_floats(game::triangle<game::vertex_attributes_only> const &t)
  {
    using X = game::triangle<game::vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{t.bottom_left.vertex.x,  t.bottom_left.vertex.y,
                                                t.bottom_left.vertex.z,  t.bottom_left.vertex.w,

                                                t.bottom_right.vertex.x, t.bottom_right.vertex.y,
                                                t.bottom_right.vertex.z, t.bottom_right.vertex.w,

                                                t.top_middle.vertex.x,   t.top_middle.vertex.y,
                                                t.top_middle.vertex.z,   t.top_middle.vertex.w};
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(t.draw_mode());
    return vertex_only_triangle{mode, std::move(floats), TRIANGLE_ELEMENTS()};
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // rectangles
  static auto constexpr RECTANGLE_VERTEX_ORDERING()
  {
    return std::array<ElementType, 6>{0, 1, 2, 2, 3, 0};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_color_attributes> const &r)
  {
    using X = game::rectangle<game::vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
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
        r.top_left.color.b,      r.top_left.color.a};
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_color_rectangle{mode, std::move(floats), RECTANGLE_VERTEX_ORDERING()};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_uv_attributes> const &r)
  {
    using X = game::rectangle<game::vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_uv_num_floats(NUM_VERTICES);

    auto const &bl = r.bottom_left;
    auto const &br = r.bottom_right;
    auto const &tr = r.top_right;
    auto const &tl = r.top_left;

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        bl.vertex.x, bl.vertex.y, bl.vertex.z, bl.vertex.w, bl.uv.u, bl.uv.v,
        br.vertex.x, br.vertex.y, br.vertex.z, br.vertex.w, br.uv.u, br.uv.v,
        tr.vertex.x, tr.vertex.y, tr.vertex.z, tr.vertex.w, tr.uv.u, tr.uv.v,
        tl.vertex.x, tl.vertex.y, tl.vertex.z, tl.vertex.w, tl.uv.u, tl.uv.v,
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_uv_rectangle{mode, std::move(floats), RECTANGLE_VERTEX_ORDERING()};
  }

  static constexpr auto map_to_array_floats(game::rectangle<game::vertex_attributes_only> const &r)
  {
    using X = game::rectangle<game::vertex_attributes_only>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);

    auto const &bl = r.bottom_left;
    auto const &br = r.bottom_right;
    auto const &tr = r.top_right;
    auto const &tl = r.top_left;

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        bl.vertex.x, bl.vertex.y, bl.vertex.z, bl.vertex.w,
        br.vertex.x, br.vertex.y, br.vertex.z, br.vertex.w,
        tr.vertex.x, tr.vertex.y, tr.vertex.z, tr.vertex.w,
        tl.vertex.x, tl.vertex.y, tl.vertex.z, tl.vertex.w,
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_only_rectangle{mode, std::move(floats), RECTANGLE_VERTEX_ORDERING()};
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // cubes
  static constexpr auto CUBE_VERTICE_ORDERING()
  {
    // clang-format off
    return std::array<ElementType, 14> {
      3, 2, 6, 7, 4, 2, 0,
      3, 1, 6, 5, 4, 1, 0
    };
    // clang-format on
  }

  static constexpr auto map_to_array_floats(game::cube<game::vertex_color_attributes> const &r)
  {
    using X = game::cube<game::vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);
    auto const &v = r.vertices;

    // This ordering of the vertices allows us to render the cube as a single triangle strip. This
    // ordering of vertices and vertex originates from the following paper:
    // http://www.cs.umd.edu/projects/gvil/papers/av_ts.pdf

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        v[2].vertex.x, v[2].vertex.y, v[2].vertex.z, v[2].vertex.w, // front top-right
        v[2].color.r,  v[2].color.g,  v[2].color.b,  v[2].color.a,

        v[3].vertex.x, v[3].vertex.y, v[3].vertex.z, v[3].vertex.w, // front top-left
        v[3].color.r,  v[3].color.g,  v[3].color.b,  v[3].color.a,

        v[6].vertex.x, v[6].vertex.y, v[6].vertex.z, v[6].vertex.w, // back top-right
        v[6].color.r,  v[6].color.g,  v[6].color.b,  v[6].color.a,

        v[7].vertex.x, v[7].vertex.y, v[7].vertex.z, v[7].vertex.w, // back top-left
        v[7].color.r,  v[7].color.g,  v[7].color.b,  v[7].color.a,

        v[1].vertex.x, v[1].vertex.y, v[1].vertex.z, v[1].vertex.w, // front bottom-right
        v[1].color.r,  v[1].color.g,  v[1].color.b,  v[1].color.a,

        v[0].vertex.x, v[0].vertex.y, v[0].vertex.z, v[0].vertex.w, // front bottom-left
        v[0].color.r,  v[0].color.g,  v[0].color.b,  v[0].color.a,

        v[4].vertex.x, v[4].vertex.y, v[4].vertex.z, v[4].vertex.w, // back bottom-left
        v[4].color.r,  v[4].color.g,  v[4].color.b,  v[4].color.a,

        v[5].vertex.x, v[5].vertex.y, v[5].vertex.z, v[5].vertex.w, // back bottom-right
        v[5].color.r,  v[5].color.g,  v[5].color.b,  v[5].color.a,
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_color_cube{mode, std::move(floats), CUBE_VERTICE_ORDERING(), r.model()};
  }

  static constexpr auto map_to_array_floats(game::cube<game::vertex_attributes_only> const &r)
  {
    using X = game::cube<game::vertex_attributes_only>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);
    auto const &v = r.vertices;

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        v[2].vertex.x, v[2].vertex.y, v[2].vertex.z, v[2].vertex.w, // front top-right

        v[3].vertex.x, v[3].vertex.y, v[3].vertex.z, v[3].vertex.w, // front top-left

        v[6].vertex.x, v[6].vertex.y, v[6].vertex.z, v[6].vertex.w, // back top-right

        v[7].vertex.x, v[7].vertex.y, v[7].vertex.z, v[7].vertex.w, // back top-left

        v[1].vertex.x, v[1].vertex.y, v[1].vertex.z, v[1].vertex.w, // front bottom-right

        v[0].vertex.x, v[0].vertex.y, v[0].vertex.z, v[0].vertex.w, // front bottom-left

        v[4].vertex.x, v[4].vertex.y, v[4].vertex.z, v[4].vertex.w, // back bottom-left

        v[5].vertex.x, v[5].vertex.y, v[5].vertex.z, v[5].vertex.w, // back bottom-right
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_only_cube{mode, std::move(floats), CUBE_VERTICE_ORDERING(), r.model()};
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // polygon
  template <typename R, typename P, typename CountFN, typename FN>
  static auto map_polygon(P const& p, CountFN const& count, FN const& fill_vertice)
  {
    auto const num_floats = count(p.num_vertices());

    stlw::sized_buffer<FloatType> floats{static_cast<size_t>(num_floats)};
    for (auto i{0}, j{0}; j < floats.length(); ++i) {
      fill_vertice(floats, i, j);
    }
    auto const ordering_length = p.num_vertices();
    stlw::sized_buffer<ElementType> vertex_ordering{static_cast<size_t>(ordering_length)};
    for (auto i{0}; i < ordering_length; ++i) {
      vertex_ordering[i] = i;
    }
    auto const mode = map_gfx_mode_to_opengl_mode(p.draw_mode());
    return R{mode, std::move(floats), std::move(vertex_ordering)};
  }

  static auto map_to_array_floats(game::polygon<game::vertex_color_attributes> const &p)
  {
    auto const fill_vertice = [&p](auto &floats, auto const i, auto &j) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertice.vertex.x;
      floats[j++] = vertice.vertex.y;
      floats[j++] = vertice.vertex.z;
      floats[j++] = vertice.vertex.w;

      floats[j++] = vertice.color.r;
      floats[j++] = vertice.color.g;
      floats[j++] = vertice.color.b;
      floats[j++] = vertice.color.a;
    };
    return map_polygon<vertex_color_polygon>(p, &calc_vertex_color_num_floats, fill_vertice);
  }

  static auto map_to_array_floats(game::polygon<game::vertex_attributes_only> const &p)
  {
    auto const fill_vertice = [&p](auto &floats, auto const i, auto &j) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertice.vertex.x;
      floats[j++] = vertice.vertex.y;
      floats[j++] = vertice.vertex.z;
      floats[j++] = vertice.vertex.w;
    };
    return map_polygon<vertex_only_polygon>(p, &calc_vertex_only_num_floats, fill_vertice);
  }

  static auto map_to_array_floats(game::polygon<game::vertex_uv_attributes> const &p)
  {
    auto const fill_vertice = [&p](auto &floats, auto const i, auto &j) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertice.vertex.x;
      floats[j++] = vertice.vertex.y;
      floats[j++] = vertice.vertex.z;
      floats[j++] = vertice.vertex.w;

      floats[j++] = vertice.uv.u;
      floats[j++] = vertice.uv.v;
    };
    return map_polygon<vertex_uv_polygon>(p, &calc_vertex_uv_num_floats, fill_vertice);
  }

public:
  template <typename... T, typename... R>
  static constexpr auto map_to_opengl(std::tuple<T...> const& shapes)
  {
    auto fn = [&](auto const& s) { return shape_mapper::map_to_array_floats(s); };
    return stlw::map_tuple_elements(shapes, fn);
  }
};

} // ns engine::gfx::opengl
