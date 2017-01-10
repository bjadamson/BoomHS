#pragma once
#include <opengl/vertex_attribute.hpp>
#include <opengl/mode.hpp>
#include <opengl/shape2d.hpp>
#include <opengl/shape3d.hpp>
#include <glm/glm.hpp>
#include <stlw/burrito.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_ctors.hpp>

namespace opengl
{

using namespace opengl;

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
      , vertices(MOVE(v))
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

  opengl::model const& model;

  MOVE_DEFAULT(shape3d_data);
  NO_COPY(shape3d_data);
  explicit constexpr shape3d_data(GLenum const m, VertexContainer &&d, VertexOrdering const& e,
      opengl::model const& model)
    : BASE(m, MOVE(d), e)
    , model(model)
  {
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// runtime-driven data
template <template <class, class> typename C, std::size_t NUM_OF_F_PER_VERTEX,
         typename A = std::allocator<FloatType>, typename B = std::allocator<ElementType>>
struct runtime_2dshape_template
{
  GLenum const draw_mode;
  C<FloatType, A> vertices;
  C<ElementType, B> ordering;

  NO_COPY(runtime_2dshape_template);
  MOVE_DEFAULT(runtime_2dshape_template);
  explicit runtime_2dshape_template(GLenum const dm, C<FloatType, A> &&d, C<ElementType, B> &&e)
      : draw_mode(dm)
      , vertices(MOVE(d))
      , ordering(MOVE(e))
  {
  }
  auto constexpr num_floats_per_vertex() const { return NUM_OF_F_PER_VERTEX; }
};

template <template <class, class> typename C, std::size_t NUM_OF_F_PER_VERTEX,
         typename A = std::allocator<FloatType>, typename B = std::allocator<ElementType>>
struct runtime_3dshape_template
{
  GLenum const draw_mode;
  C<FloatType, A> vertices;
  C<ElementType, B> ordering;
  opengl::model const& model;

  NO_COPY(runtime_3dshape_template);
  MOVE_DEFAULT(runtime_3dshape_template);
  explicit runtime_3dshape_template(GLenum const dm, C<FloatType, A> &&d, C<ElementType, B> &&e,
      opengl::model const& m)
    : draw_mode(dm)
    , vertices(MOVE(d))
    , ordering(MOVE(e))
    , model(m)
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
using runtime_sized_2dshape = runtime_2dshape_template<stlw::sized_buffer, NUM_VERTEXES>;

template <std::size_t NUM_VERTEXES>
using runtime_sized_3dshape = runtime_3dshape_template<stlw::sized_buffer, NUM_VERTEXES>;

// float polygons
using vertex_color_polygon = runtime_sized_2dshape<8>;
using vertex_uv_polygon = runtime_sized_2dshape<7>;
using vertex_only_polygon = runtime_sized_2dshape<4>;

// meshs
using vertex_color_mesh = runtime_sized_3dshape<8>;
using vertex_uv_mesh = runtime_sized_3dshape<7>;
using vertex_only_mesh = runtime_sized_3dshape<4>;

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
  static constexpr auto map_to_array_floats(triangle<vertex_color_attributes> const &t)
  {
    using X = triangle<vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        vertex(t.bottom_left).x,  vertex(t.bottom_left).y,  vertex(t.bottom_left).z,
        vertex(t.bottom_left).w,  color(t.bottom_left).r,  color(t.bottom_left).g,
        color(t.bottom_left).b,   color(t.bottom_left).a,

        vertex(t.bottom_right).x, vertex(t.bottom_right).y, vertex(t.bottom_right).z,
        vertex(t.bottom_right).w, color(t.bottom_right).r, color(t.bottom_right).g,
        color(t.bottom_right).b,  color(t.bottom_right).a,

        vertex(t.top_middle).x,   vertex(t.top_middle).y, vertex(t.top_middle).z,
        vertex(t.top_middle).w,   color(t.top_middle).r,  color(t.top_middle).g,
        color(t.top_middle).b,    color(t.top_middle).a
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(t.draw_mode());
    return vertex_color_triangle{mode, MOVE(floats), TRIANGLE_ELEMENTS()};
  }

  static constexpr auto map_to_array_floats(triangle<vertex_uv_attributes> const &t)
  {
    using X = triangle<vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_uv_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        vertex(t.bottom_left).x,  vertex(t.bottom_left).y,  vertex(t.bottom_left).z,
        vertex(t.bottom_left).w,  uv(t.bottom_left).u,      uv(t.bottom_left).v,

        vertex(t.bottom_right).x, vertex(t.bottom_right).y, vertex(t.bottom_right).z,
        vertex(t.bottom_right).w, uv(t.bottom_right).u,     uv(t.bottom_right).v,

        vertex(t.top_middle).x,   vertex(t.top_middle).y,   vertex(t.top_middle).z,
        vertex(t.top_middle).w,   uv(t.top_middle).u,       uv(t.top_middle).v};
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(t.draw_mode());
    return vertex_uv_triangle{mode, MOVE(floats), TRIANGLE_ELEMENTS()};
  }

  static constexpr auto map_to_array_floats(triangle<vertex_attributes_only> const &t)
  {
    using X = triangle<vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
      vertex(t.bottom_left).x,  vertex(t.bottom_left).y, vertex(t.bottom_left).z,  vertex(t.bottom_left).w,
      vertex(t.bottom_right).x, vertex(t.bottom_right).y, vertex(t.bottom_right).z,   vertex(t.bottom_right).w,
      vertex(t.top_middle).x,   vertex(t.top_middle).y, vertex(t.top_middle).z, vertex(t.top_middle).w
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(t.draw_mode());
    return vertex_only_triangle{mode, MOVE(floats), TRIANGLE_ELEMENTS()};
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // rectangles
  static auto constexpr RECTANGLE_VERTEX_ORDERING()
  {
    return std::array<ElementType, 6>{0, 1, 2, 2, 3, 0};
  }

  static constexpr auto map_to_array_floats(rectangle<vertex_color_attributes> const &r)
  {
    using X = rectangle<vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        vertex(r.bottom_left).x, vertex(r.bottom_left).y, vertex(r.bottom_left).z, vertex(r.bottom_left).w,
        color(r.bottom_left).r,  color(r.bottom_left).g,  color(r.bottom_left).b,  color(r.bottom_left).a,

        vertex(r.bottom_right).x, vertex(r.bottom_right).y, vertex(r.bottom_right).z, vertex(r.bottom_right).w,
        color(r.bottom_right).r,  color(r.bottom_right).g,  color(r.bottom_right).b,  color(r.bottom_right).a,

        vertex(r.top_right).x, vertex(r.top_right).y, vertex(r.top_right).z, vertex(r.top_right).w,
        color(r.top_right).r,  color(r.top_right).g,  color(r.top_right).b,  color(r.top_right).a,

        vertex(r.top_left).x, vertex(r.top_left).y, vertex(r.top_left).z, vertex(r.top_left).w,
        color(r.top_left).r,  color(r.top_left).g,  color(r.top_left).b,  color(r.top_left).a,
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_color_rectangle{mode, MOVE(floats), RECTANGLE_VERTEX_ORDERING()};
  }

  static constexpr auto map_to_array_floats(rectangle<vertex_uv_attributes> const &r)
  {
    using X = rectangle<vertex_uv_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_uv_num_floats(NUM_VERTICES);

    auto const &bl = r.bottom_left;
    auto const &br = r.bottom_right;
    auto const &tr = r.top_right;
    auto const &tl = r.top_left;

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        vertex(bl).x, vertex(bl).y, vertex(bl).z, vertex(bl).w, uv(bl).u, uv(bl).v,
        vertex(br).x, vertex(br).y, vertex(br).z, vertex(br).w, uv(br).u, uv(br).v,
        vertex(tl).x, vertex(tl).y, vertex(tl).z, vertex(tl).w, uv(tl).u, uv(tl).v,
        vertex(tr).x, vertex(tr).y, vertex(tr).z, vertex(tr).w, uv(tr).u, uv(tr).v
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_uv_rectangle{mode, MOVE(floats), RECTANGLE_VERTEX_ORDERING()};
  }

  static constexpr auto map_to_array_floats(rectangle<vertex_attributes_only> const &r)
  {
    using X = rectangle<vertex_attributes_only>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);

    auto const &bl = r.bottom_left;
    auto const &br = r.bottom_right;
    auto const &tr = r.top_right;
    auto const &tl = r.top_left;

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        vertex(bl).x, vertex(bl).y, vertex(bl).z, vertex(bl).w,
        vertex(br).x, vertex(br).y, vertex(br).z, vertex(br).w,
        vertex(tr).x, vertex(tr).y, vertex(tr).z, vertex(tr).w,
        vertex(tl).x, vertex(tl).y, vertex(tl).z, vertex(tl).w,
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_only_rectangle{mode, MOVE(floats), RECTANGLE_VERTEX_ORDERING()};
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

  static constexpr auto map_to_array_floats(cube<vertex_color_attributes> const &r)
  {
    using X = cube<vertex_color_attributes>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_color_num_floats(NUM_VERTICES);
    auto const &v = r.vertices;

    // This ordering of the vertices allows us to render the cube as a single triangle strip. This
    // ordering of vertices and vertex originates from the following paper:
    // http://www.cs.umd.edu/projects/gvil/papers/av_ts.pdf

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        vertex(v[2]).x, vertex(v[2]).y, vertex(v[2]).z, vertex(v[2]).w,
        color(v[2]).r,  color(v[2]).g,  color(v[2]).b,  color(v[2]).a,

        vertex(v[3]).x, vertex(v[3]).y, vertex(v[3]).z, vertex(v[3]).w,
        color(v[3]).r,  color(v[3]).g,  color(v[3]).b,  color(v[3]).a,

        vertex(v[6]).x, vertex(v[6]).y, vertex(v[6]).z, vertex(v[6]).w,
        color(v[6]).r,  color(v[6]).g,  color(v[6]).b,  color(v[6]).a,

        vertex(v[7]).x, vertex(v[7]).y, vertex(v[7]).z, vertex(v[7]).w,
        color(v[7]).r,  color(v[7]).g,  color(v[7]).b,  color(v[7]).a,

        vertex(v[1]).x, vertex(v[1]).y, vertex(v[1]).z, vertex(v[1]).w,
        color(v[1]).r,  color(v[1]).g,  color(v[1]).b,  color(v[1]).a,

        vertex(v[0]).x, vertex(v[0]).y, vertex(v[0]).z, vertex(v[0]).w,
        color(v[0]).r,  color(v[0]).g,  color(v[0]).b,  color(v[0]).a,

        vertex(v[4]).x, vertex(v[4]).y, vertex(v[4]).z, vertex(v[4]).w,
        color(v[4]).r,  color(v[4]).g,  color(v[4]).b,  color(v[4]).a,

        vertex(v[5]).x, vertex(v[5]).y, vertex(v[5]).z, vertex(v[5]).w,
        color(v[5]).r,  color(v[5]).g,  color(v[5]).b,  color(v[5]).a
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_color_cube{mode, MOVE(floats), CUBE_VERTICE_ORDERING(), r.model()};
  }

  static constexpr auto map_to_array_floats(cube<vertex_attributes_only> const &r)
  {
    using X = cube<vertex_attributes_only>;
    auto constexpr NUM_VERTICES = X::NUM_VERTICES;
    auto constexpr NUM_FLOATS = calc_vertex_only_num_floats(NUM_VERTICES);
    auto const &v = r.vertices;

    // clang-format off
    auto floats = std::array<FloatType, NUM_FLOATS>{
        vertex(v[2]).x, vertex(v[2]).y, vertex(v[2]).z, vertex(v[2]).w,
        vertex(v[3]).x, vertex(v[3]).y, vertex(v[3]).z, vertex(v[3]).w,
        vertex(v[6]).x, vertex(v[6]).y, vertex(v[6]).z, vertex(v[6]).w,
        vertex(v[7]).x, vertex(v[7]).y, vertex(v[7]).z, vertex(v[7]).w,
        vertex(v[1]).x, vertex(v[1]).y, vertex(v[1]).z, vertex(v[1]).w,
        vertex(v[0]).x, vertex(v[0]).y, vertex(v[0]).z, vertex(v[0]).w,
        vertex(v[4]).x, vertex(v[4]).y, vertex(v[4]).z, vertex(v[4]).w,
        vertex(v[5]).x, vertex(v[5]).y, vertex(v[5]).z, vertex(v[5]).w,
    };
    // clang-format on
    auto const mode = map_gfx_mode_to_opengl_mode(r.draw_mode());
    return vertex_only_cube{mode, MOVE(floats), CUBE_VERTICE_ORDERING(), r.model()};
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
    return R{mode, MOVE(floats), MOVE(vertex_ordering)};
  }

  static auto map_to_array_floats(polygon<vertex_color_attributes> const &p)
  {
    auto const fill_vertice = [&p](auto &floats, auto const i, auto &j) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertex(vertice).x;
      floats[j++] = vertex(vertice).y;
      floats[j++] = vertex(vertice).z;
      floats[j++] = vertex(vertice).w;

      floats[j++] = color(vertice).r;
      floats[j++] = color(vertice).g;
      floats[j++] = color(vertice).b;
      floats[j++] = color(vertice).a;
    };
    return map_polygon<vertex_color_polygon>(p, &calc_vertex_color_num_floats, fill_vertice);
  }

  static auto map_to_array_floats(polygon<vertex_attributes_only> const &p)
  {
    auto const fill_vertice = [&p](auto &floats, auto const i, auto &j) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertex(vertice).x;
      floats[j++] = vertex(vertice).y;
      floats[j++] = vertex(vertice).z;
      floats[j++] = vertex(vertice).w;
    };
    return map_polygon<vertex_only_polygon>(p, &calc_vertex_only_num_floats, fill_vertice);
  }

  static auto map_to_array_floats(polygon<vertex_uv_attributes> const &p)
  {
    auto const fill_vertice = [&p](auto &floats, auto const i, auto &j) {
      auto &vertice = p.vertex_attributes[i];
      floats[j++] = vertex(vertice).x;
      floats[j++] = vertex(vertice).y;
      floats[j++] = vertex(vertice).z;
      floats[j++] = vertex(vertice).w;

      floats[j++] = uv(vertice).u;
      floats[j++] = uv(vertice).v;
    };
    return map_polygon<vertex_uv_polygon>(p, &calc_vertex_uv_num_floats, fill_vertice);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mesh
  template <typename R, typename M, typename FN>
  static auto map_mesh(M const& mesh, FN const& fill_vertice)
  {
    auto const num_floats = mesh.object_data.buffer.size();// * 8; // 3x pos +  3x normals

    stlw::sized_buffer<FloatType> floats{static_cast<size_t>(num_floats)};
    for (auto i{0}, j{0}; j < floats.length(); ++i) {
      fill_vertice(floats, i, j);
    }
    auto const ordering_length = mesh.object_data.indices.size();
    stlw::sized_buffer<ElementType> vertex_ordering{static_cast<size_t>(ordering_length)};
    for (auto i{0}; i < ordering_length; ++i) {
      vertex_ordering[i] = i;//mesh.object_data.indices[i];
    }
    auto const mode = map_gfx_mode_to_opengl_mode(mesh.draw_mode());
    return R{mode, MOVE(floats), MOVE(vertex_ordering), mesh.model()};
  }

  static auto map_to_array_floats(mesh<vertex_color_attributes> const &m)
  {
    auto const fill_vertice = [&m](auto &floats, auto const i, auto &j) {
      auto &vertice = m.vertex_attributes[i];
      floats[j++] = vertex(vertice).x;
      floats[j++] = vertex(vertice).y;
      floats[j++] = vertex(vertice).z;
      floats[j++] = vertex(vertice).w;

      floats[j++] = color(vertice).r;
      floats[j++] = color(vertice).g;
      floats[j++] = color(vertice).b;
      floats[j++] = color(vertice).a;
    };
    return map_mesh<vertex_color_mesh>(m, fill_vertice);
  }

public:
  template <typename SHAPE, typename... R>
  static auto constexpr map_to_opengl(SHAPE const& shape)
  {
    auto fn = [&](auto const& s) { return shape_mapper::map_to_array_floats(s); };
    return fn(shape);
  }
};

} // ns opengl
