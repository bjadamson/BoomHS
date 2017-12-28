#pragma once
#include <array>
#include <cmath>
#include <glm/gtx/vector_query.hpp>
#include <glm/gtc/epsilon.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/obj.hpp>

#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>

#include <boomhs/types.hpp>

namespace opengl
{

struct MeshProperties
{
  obj const& object_data;

  explicit MeshProperties(obj const& obj)
    : object_data(obj)
  {
  }
};

namespace cube_factory
{

using CubeDimensions = boomhs::WidthHeightLength;

struct CubeProperties
{
  CubeDimensions const dimensions;
  bool wireframe = false;
};

struct ColorProperties {
    std::array<Color, 8> const colors;
  };

struct UVProperties {
};

struct WireframeProperties {
  float const alpha = 1.0f;
};

auto
construct_cube(std::array<float, 32> const& vertices, ColorProperties const &props)
{
  // clang-format off
  auto const& colors = props.colors;
  return stlw::make_array<float>(
      vertices[0], vertices[1], vertices[2], vertices[3],
      colors[0].r, colors[0].g, colors[0].a, colors[0].a,

      vertices[4], vertices[5], vertices[6], vertices[7],
      colors[1].r, colors[1].g, colors[1].a, colors[1].a,

      vertices[8], vertices[9], vertices[10], vertices[11],
      colors[2].r, colors[2].g, colors[2].a, colors[2].a,

      vertices[12], vertices[13], vertices[14], vertices[15],
      colors[3].r, colors[3].g, colors[3].a, colors[3].a,

      vertices[16], vertices[17], vertices[18], vertices[19],
      colors[4].r, colors[4].g, colors[4].a, colors[4].a,

      vertices[20], vertices[21], vertices[22], vertices[23],
      colors[5].r, colors[5].g, colors[5].a, colors[5].a,

      vertices[24], vertices[25], vertices[26], vertices[27],
      colors[6].r, colors[6].g, colors[6].a, colors[6].a,

      vertices[28], vertices[29], vertices[30], vertices[31],
      colors[7].r, colors[7].g, colors[7].a, colors[7].a,

      vertices[32], vertices[33], vertices[34], vertices[35],
      colors[8].r, colors[8].g, colors[8].a, colors[8].a
        );
  // clang-format on
}

auto
construct_cube(std::array<float, 32> const& vertices, UVProperties const &props)
{
  // clang-format off
  return stlw::make_array<float>(
      vertices[0], vertices[1], vertices[2], vertices[3],
      vertices[4], vertices[5], vertices[6], vertices[7],
      vertices[8], vertices[9], vertices[10], vertices[11],
      vertices[12], vertices[13], vertices[14], vertices[15],
      vertices[16], vertices[17], vertices[18], vertices[19],
      vertices[20], vertices[21], vertices[22], vertices[23],
      vertices[24], vertices[25], vertices[26], vertices[27],
      vertices[28], vertices[29], vertices[30], vertices[31],
      vertices[32], vertices[33], vertices[34], vertices[35]
      );
  // clang-format on
}

auto
construct_cube(std::array<float, 32> const& vertices, WireframeProperties const &props)
{
  // clang-format off
  return stlw::make_array<float>(
      vertices[0], vertices[1], vertices[2], vertices[3],
      vertices[4], vertices[5], vertices[6], vertices[7],
      vertices[8], vertices[9], vertices[10], vertices[11],
      vertices[12], vertices[13], vertices[14], vertices[15],
      vertices[16], vertices[17], vertices[18], vertices[19],
      vertices[20], vertices[21], vertices[22], vertices[23],
      vertices[24], vertices[25], vertices[26], vertices[27],
      vertices[28], vertices[29], vertices[30], vertices[31],
      vertices[32], vertices[33], vertices[34], vertices[35]
      );
  // clang-format on
}

auto
make_cube(std::array<float, 32> const& vertices, boomhs::color_t, Color const& color)
{
  std::array<Color, 8> const colors{
      color, color, color, color,
      color, color, color, color
  };
  ColorProperties const p{colors};
  return construct_cube(vertices, p);
}

auto
make_cube(std::array<float, 32> const& vertices, boomhs::uv_t)
{
  UVProperties const uv;
  return construct_cube(vertices, uv);
}

auto
make_cube(std::array<float, 32> const& vertices, boomhs::wireframe_t)
{
  WireframeProperties const wf;
  return construct_cube(vertices, wf);
}

} // ns cube_factory

namespace detail
{
template<typename PIPE, typename VERTICES, typename INDICES>
void
copy_to_gpu(stlw::Logger &logger, PIPE &pipeline, DrawInfo const& dinfo, VERTICES const& vertices,
    INDICES const& indices)
{
  // Activate VAO
  global::vao_bind(pipeline.vao());

  glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinfo.ebo());

  auto const& va = pipeline.va();
  va.upload_vertex_format_to_glbound_vao(logger);

  // copy the vertices
  auto const vertices_size = vertices.size() * sizeof(GLfloat);
  auto const& vertices_data = vertices.data();
  glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

  // copy the vertice rendering order
  auto const indices_size = sizeof(GLuint) * indices.size();
  auto const& indices_data = indices.data();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);
}

} // ns detail

namespace factories
{

template<typename PIPE, typename ...Args>
auto copy_cube_gpu(stlw::Logger &logger, PIPE &pipeline, cube_factory::CubeProperties const& cprop,
    Args &&... args)
{
  // clang-format off
  static constexpr std::array<GLuint, 36> INDICES = {{
    0, 1, 2,  2, 3, 0, // front
    1, 5, 6,  6, 2, 1, // top
    7, 6, 5,  5, 4, 7, // back
    4, 0, 3,  3, 7, 4, // bottom
    4, 5, 1,  1, 0, 4, // left
    3, 2, 6,  6, 7, 3, // right
  }};
  // clang-format on

  using factory_type = typename PIPE::info_t;

  auto const& hw = cprop.dimensions;
  auto const h = hw.height;
  auto const w = hw.width;
  auto const l = hw.length;

  // Define the 8 vertices of a unit cube
  float constexpr W = 1.0f;
  static const std::array<float, 32> v = stlw::make_array<float>(
      // front
    -1.0f, -1.0f,  1.0f, W,
     1.0f, -1.0f,  1.0f, W,
     1.0f,  1.0f,  1.0f, W,
    -1.0f,  1.0f,  1.0f, W,
    // back
    -1.0f, -1.0f, -1.0f, W,
     1.0f, -1.0f, -1.0f, W,
     1.0f,  1.0f, -1.0f, W,
    -1.0f,  1.0f, -1.0f, W
  );

  auto const vertices = cube_factory::make_cube(v, factory_type{}, std::forward<Args>(args)...);

  GLenum const draw_mode = cprop.wireframe ? GL_LINE_LOOP : GL_TRIANGLES;
  DrawInfo dinfo{draw_mode, INDICES.size()};
  detail::copy_to_gpu(logger, pipeline, dinfo, vertices, INDICES);
  return dinfo;
}

template<typename PIPE, typename ...Args>
auto copy_cube_14indices_gpu(stlw::Logger &logger, PIPE &pipeline, cube_factory::CubeProperties const& cprop,
    Args &&... args)
{
  // clang-format off
  static constexpr std::array<GLuint, 14> INDICES = {{
    3, 2, 6, 7, 4, 2, 0,
    3, 1, 6, 5, 4, 1, 0
  }};
  // clang-format on

  using factory_type = typename PIPE::info_t;

  auto const& hw = cprop.dimensions;
  auto const h = hw.height;
  auto const w = hw.width;
  auto const l = hw.length;

  // clang-format off
  auto const arr = stlw::make_array<float>(
   -w, -h, l, 1.0f, // front bottom-left
    w, -h, l, 1.0f, // front bottom-right
    w,  h, l, 1.0f, // front top-right
   -w,  h, l, 1.0f, // front top-left

    -w, -h, -l, 1.0f, // back bottom-left
     w, -h, -l, 1.0f, // back bottom-right
     w,  h, -l, 1.0f, // back top-right
    -w,  h, -l, 1.0f  // back top-left
  );

  auto const v = stlw::make_array<float>(
      arr[8], arr[9], arr[10], arr[11],   // CUBE_ROW_2
      arr[12], arr[13], arr[14], arr[15], // CUBE_ROW_3
      arr[24], arr[25], arr[26], arr[27], // CUBE_ROW_6
      arr[28], arr[29], arr[30], arr[31], // CUBE_ROW_7

      arr[4], arr[5], arr[6], arr[7],     // CUBE_ROW_1,
      arr[0], arr[1], arr[2], arr[3],     // CUBE_ROW_0,
      arr[16], arr[17], arr[18], arr[19], // CUBE_ROW_4,
      arr[20], arr[21], arr[22], arr[23]  // CUBE_ROW_5
      );
  auto const vertices = cube_factory::make_cube(v, factory_type{}, std::forward<Args>(args)...);

  DrawInfo dinfo{GL_TRIANGLE_STRIP, INDICES.size()};
  detail::copy_to_gpu(logger, pipeline, dinfo, vertices, INDICES);
  return dinfo;
}

struct ArrowCreateParams
{
  Color const& color;

  glm::vec3 const& start;
  glm::vec3 const& end;

  float const tip_length_factor = 4.0f;
};

template<typename PIPE>
auto
create_arrow(stlw::Logger &logger, PIPE &pipeline, ArrowCreateParams &&params)
{
  auto const adjust_if_zero = [=](glm::vec3 const& v) {
    auto constexpr ZERO_VEC = glm::zero<glm::vec3>();
    auto constexpr EPSILON = std::numeric_limits<float>::epsilon();
    auto constexpr EPSILON_VEC = glm::vec3{EPSILON, EPSILON, EPSILON};
    bool const is_zero = glm::all(glm::epsilonEqual(v, ZERO_VEC, EPSILON));
    return is_zero ? EPSILON_VEC : v;
  };

  // Normalizing a zero vector is undefined. Therefore if the user passes us a zero vector, since
  // we are creating an arrow, pretend the point is EPSILON away from the true origin (so
  // normalizing the crossproduct doesn't yield vector's with NaN for their components).
  auto const A = adjust_if_zero(params.start);
  auto const B = adjust_if_zero(params.end);

  glm::vec3 const v = A - B;
  glm::vec3 const rev = -v;

  glm::vec3 const cross1 = glm::normalize(glm::cross(A, B));
  glm::vec3 const cross2 = glm::normalize(glm::cross(B, A));

  glm::vec3 const vp1 = glm::normalize(rev + cross1);
  glm::vec3 const vp2 = glm::normalize(rev + cross2) ;

  float const factor = params.tip_length_factor;
  glm::vec3 const p1 = B - (vp1 / factor);
  glm::vec3 const p2 = B - (vp2 / factor);

#define COLOR params.color.r, params.color.g, params.color.b, params.color.a
#define START params.start.x, params.start.y, params.start.z, 1.0f
#define END params.end.x, params.end.y, params.end.z, 1.0f
#define P1 p1.x, p1.y, p1.z, 1.0f
#define P2 p2.x, p2.y, p2.z, 1.0f
  auto const vertices = stlw::make_array<float>(
      // START -> END
      START, COLOR,
      END, COLOR,

      // END -> P1
      END, COLOR,
      P1, COLOR,

      // END -> P2
      END, COLOR,
      P2, COLOR
      );
#undef COLOR
#undef START
#undef END
#undef P1
#undef P2
  static constexpr std::array<GLuint, 6> INDICES = {{
    0, 1, 2, 3, 4, 5
  }};

  DrawInfo dinfo{GL_LINES, INDICES.size()};
  detail::copy_to_gpu(logger, pipeline, dinfo, vertices, INDICES);
  return dinfo;
}

struct WorldOriginArrows {
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

template<typename X_PIPE, typename Y_PIPE, typename Z_PIPE>
WorldOriginArrows
create_axis_arrows(stlw::Logger &logger, X_PIPE &x_pipe, Y_PIPE &y_pipe, Z_PIPE &z_pipe,
    glm::vec3 const& origin)
{
  auto x = create_arrow(logger, x_pipe, ArrowCreateParams{LOC::RED, origin, origin + X_UNIT_VECTOR});
  auto y = create_arrow(logger, y_pipe, ArrowCreateParams{LOC::GREEN, origin, origin + Y_UNIT_VECTOR});
  auto z = create_arrow(logger, z_pipe, ArrowCreateParams{LOC::BLUE, origin, origin + Z_UNIT_VECTOR});
  return WorldOriginArrows{MOVE(x), MOVE(y), MOVE(z)};
}

template<typename X_PIPE, typename Y_PIPE, typename Z_PIPE>
WorldOriginArrows
create_world_axis_arrows(stlw::Logger &logger, X_PIPE &x_pipe, Y_PIPE &y_pipe, Z_PIPE &z_pipe)
{
  glm::vec3 constexpr ORIGIN = glm::zero<glm::vec3>();
  return create_axis_arrows(logger, x_pipe, y_pipe, z_pipe, ORIGIN);
}

template<typename P, typename ...Args>
auto make_mesh(stlw::Logger &logger, P &pipeline, MeshProperties &&mprop, Args &&... args)
{
  auto const& indices = mprop.object_data.indices;
  auto const& vertices = mprop.object_data.vertices;

  auto const num_indices = static_cast<GLuint>(mprop.object_data.indices.size());
  DrawInfo dinfo{GL_TRIANGLES, num_indices};

  detail::copy_to_gpu(logger, pipeline, dinfo, vertices, indices);
  return dinfo;
}

struct TilemapProperties
{
  GLenum const draw_mode;

  obj const& hashtag;
};

auto copy_tilemap_gpu(stlw::Logger &logger, PipelineHashtag3D &hash_pipe,
    TilemapProperties &&tprops)
{
  auto const& vertices = tprops.hashtag.vertices;
  auto const& indices = tprops.hashtag.indices;

  // assume (x, y, z, w) all present
  // assume (r, g, b, a) all present
  assert((vertices.size() % 8) == 0);

  // Bind the vao (even before instantiating the DrawInfo)
  global::vao_bind(hash_pipe.vao());

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{tprops.draw_mode, num_indices};
  auto const ebo = dinfo.ebo();
  auto const vbo = dinfo.vbo();

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  // Enable the VertexAttributes for this pipeline's VAO.
  auto const& va = hash_pipe.va();
  va.upload_vertex_format_to_glbound_vao(logger);

  // Calculate how much room the buffers need.
  std::size_t const vertices_num_bytes = (vertices.size() * sizeof(GLfloat));
  glBufferData(GL_ARRAY_BUFFER, vertices_num_bytes, vertices.data(), GL_STATIC_DRAW);

  std::size_t const indices_num_bytes = (indices.size() * sizeof(GLuint));
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_num_bytes, indices.data(), GL_STATIC_DRAW);
  return dinfo;
}

} // ns factories

} // ns opengl
