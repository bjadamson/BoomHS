#include <opengl/factory.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/obj.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>

#include <boomhs/tilemap.hpp>
#include <boomhs/types.hpp>

#include <glm/gtx/vector_query.hpp>
#include <glm/gtc/epsilon.hpp>

#include <boost/optional.hpp>
#include <array>
#include <cmath>

using namespace opengl;
using namespace opengl::factories;

namespace
{

template<typename INDICES, typename VERTICES>
void
copy_to_gpu(stlw::Logger &logger, ShaderProgram const& shader_program, DrawInfo const& dinfo, VERTICES const& vertices,
    INDICES const& indices)
{
  // Activate VAO
  global::vao_bind(dinfo.vao());

  glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinfo.ebo());

  auto const& va = shader_program.va();
  va.upload_vertex_format_to_glbound_vao(logger);

  // copy the vertices
  auto const vertices_size = vertices.size() * sizeof(GLfloat);
  auto const& vertices_data = vertices.data();
  LOG_TRACE(fmt::format("inserting '%i' vertices into GL_BUFFER_ARRAY\n", vertices.size()));
  glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

  // copy the vertice rendering order
  auto const indices_size = sizeof(GLuint) * indices.size();
  auto const& indices_data = indices.data();
  LOG_TRACE(fmt::format("inserting '%i' indices into GL_ELEMENT_BUFFER_ARRAY\n", indices.size()));
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);
}

auto
cube_vertices()
{
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
  return v;
}

template<std::size_t N>
DrawInfo
make_cube_drawinfo(stlw::Logger &logger, std::array<float, N> const& vertex_data,
    ShaderProgram const& shader_program, boost::optional<TextureInfo> const& ti)
{
  DrawInfo dinfo{GL_TRIANGLES, vertex_data.size(), cube_factory::INDICES.size(), ti};
  copy_to_gpu(logger, shader_program, dinfo, vertex_data, cube_factory::INDICES);
  return dinfo;
}

} // ns anonymous

namespace opengl::factories
{

DrawInfo
copy_colorcube_gpu(stlw::Logger &logger, ShaderProgram const& shader_program, Color const& color)
{
  // clang-format off
  std::array<Color, 8> const c{
      color, color, color, color,
      color, color, color, color,
  };
#define COLOR(i) c[i].r, c[i].g, c[i].b, c[i].a
#define VERTS(a, b, c, d) v[a], v[b], v[c], v[d]
  auto const v = cube_vertices();
  auto const vertex_data = std::array<float, (32 * 2)>{
    VERTS(0, 1, 2, 3),     COLOR(0),
    VERTS(4, 5, 6, 7),     COLOR(1),
    VERTS(8, 9, 10, 11),   COLOR(2),
    VERTS(12, 13, 14, 15), COLOR(3),
    VERTS(16, 17, 18, 19), COLOR(4),
    VERTS(20, 21, 22, 23), COLOR(5),
    VERTS(24, 25, 26, 27), COLOR(6),
    VERTS(28, 29, 30, 31), COLOR(7),
        };
#undef COLOR
#undef VERTS
  // clang-format on
  return make_cube_drawinfo(logger, vertex_data, shader_program, boost::none);
}

DrawInfo
copy_texturecube_gpu(stlw::Logger &logger, ShaderProgram const& shader_program, TextureInfo const& ti)
{
  // clang-format off
  auto const vertices = cube_vertices();
  auto const vertex_data = std::array<float, 32>{
      vertices[0], vertices[1], vertices[2], vertices[3],
      vertices[4], vertices[5], vertices[6], vertices[7],
      vertices[8], vertices[9], vertices[10], vertices[11],
      vertices[12], vertices[13], vertices[14], vertices[15],
      vertices[16], vertices[17], vertices[18], vertices[19],
      vertices[20], vertices[21], vertices[22], vertices[23],
      vertices[24], vertices[25], vertices[26], vertices[27],
      vertices[28], vertices[29], vertices[30], vertices[31],
      };
  return make_cube_drawinfo(logger, vertex_data, shader_program, boost::make_optional(ti));
}

DrawInfo
copy_cube_14indices_gpu(stlw::Logger &logger, ShaderProgram const& shader_program,
    boost::optional<TextureInfo> const& ti)
{
  // clang-format off
  static constexpr std::array<GLuint, 14> INDICES = {{
    3, 2, 6, 7, 4, 2, 0,
    3, 1, 6, 5, 4, 1, 0
  }};
  // clang-format on

  // clang-format off
  auto const arr = stlw::make_array<float>(
   -1.0f, -1.0f, 1.0f, 1.0f, // front bottom-left
    1.0f, -1.0f, 1.0f, 1.0f, // front bottom-right
    1.0f,  1.0f, 1.0f, 1.0f, // front top-right
   -1.0f,  1.0f, 1.0f, 1.0f, // front top-left

    -1.0f, -1.0f, -1.0f, 1.0f, // back bottom-left
     1.0f, -1.0f, -1.0f, 1.0f, // back bottom-right
     1.0f,  1.0f, -1.0f, 1.0f, // back top-right
    -1.0f,  1.0f, -1.0f, 1.0f  // back top-left
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
  auto const& vertices = v;

  DrawInfo dinfo{GL_TRIANGLE_STRIP, vertices.size(), INDICES.size(), ti};
  copy_to_gpu(logger, shader_program, dinfo, vertices, INDICES);
  return dinfo;
}

auto
calculate_arrow_endpoints(ArrowCreateParams const& params)
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

  return ArrowEndpoints{p1, p2};
}

auto
make_arrow_vertices(ArrowCreateParams const& params, ArrowEndpoints const& endpoints)
{
  auto const& p1 = endpoints.p1, p2 = endpoints.p2;
#define COLOR params.color.r, params.color.g, params.color.b, params.color.a
#define START params.start.x, params.start.y, params.start.z, 1.0f
#define END params.end.x, params.end.y, params.end.z, 1.0f
#define P1 p1.x, p1.y, p1.z, 1.0f
#define P2 p2.x, p2.y, p2.z, 1.0f
  return stlw::make_array<float>(
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
}

DrawInfo
create_arrow_2d(stlw::Logger &logger, ShaderProgram const& shader_program, ArrowCreateParams &&params)
{
  //params.start.z = 0.0f;
  //params.end.z = 0.0f;

  auto endpoints = calculate_arrow_endpoints(params);
  //endpoints.p1.z = 0.0f;
  //endpoints.p2.z = 0.0f;
  auto const vertices = make_arrow_vertices(params, endpoints);

  static constexpr std::array<GLuint, 6> INDICES = {{
    0, 1, 2, 3, 4, 5
  }};

  DrawInfo dinfo{GL_LINES, vertices.size(), INDICES.size(), boost::none};
  copy_to_gpu(logger, shader_program, dinfo, vertices, INDICES);
  return dinfo;
}

DrawInfo
create_arrow(stlw::Logger &logger, ShaderProgram const& shader_program, ArrowCreateParams &&params)
{
  auto const endpoints = calculate_arrow_endpoints(params);
  auto const vertices = make_arrow_vertices(params, endpoints);

  static constexpr std::array<GLuint, 6> INDICES = {{
    0, 1, 2, 3, 4, 5
  }};

  DrawInfo dinfo{GL_LINES, vertices.size(), INDICES.size(), boost::none};
  copy_to_gpu(logger, shader_program, dinfo, vertices, INDICES);
  return dinfo;
}

DrawInfo
create_tilegrid(stlw::Logger &logger, ShaderProgram const& shader_program, boomhs::TileMap const& tmap,
    bool const show_yaxis_lines, Color const& color)
{
  std::vector<float> vertices;
  vertices.reserve(tmap.num_tiles() * 4);

  std::vector<GLuint> indices;
  indices.reserve(tmap.num_tiles());

  std::size_t count = 0u;
  auto const add_point = [&indices, &vertices, &count, &color](glm::vec3 const& point) {
    vertices.emplace_back(point.x);
    vertices.emplace_back(point.y);
    vertices.emplace_back(point.z);
    vertices.emplace_back(1.0f);

    vertices.emplace_back(color.r);
    vertices.emplace_back(color.g);
    vertices.emplace_back(color.b);
    vertices.emplace_back(color.a);

    indices.emplace_back(count++);
  };

  auto const add_line = [&add_point](glm::vec3 const& p0, glm::vec3 const& p1) {
    add_point(p0);
    add_point(p1);
  };

  auto const visit_fn = [&add_line, &show_yaxis_lines](auto const& pos) {
    auto const x = pos.x, y = pos.y, z = pos.z;
#define P0 glm::vec3{x, y, z}
#define P1 glm::vec3{x + 1, y, z}
#define P2 glm::vec3{x + 1, y + 1, z}
#define P3 glm::vec3{x, y + 1, z}

#define P4 glm::vec3{x, y, z + 1}
#define P5 glm::vec3{x + 1, y, z + 1}
#define P6 glm::vec3{x + 1, y + 1, z + 1}
#define P7 glm::vec3{x, y + 1, z + 1}
    add_line(P0, P1);
    add_line(P1, P5);
    add_line(P5, P4);
    add_line(P4, P0);

    if (show_yaxis_lines) {
      add_line(P0, P3);
      add_line(P1, P2);
      add_line(P5, P6);
      add_line(P4, P7);

      add_line(P3, P7);
      add_line(P2, P6);
      add_line(P6, P7);
      add_line(P7, P3);
    }
#undef P0
#undef P1
#undef P2
#undef P3
#undef P4
#undef P0
#undef P1
#undef P2
#undef P3
#undef P4
#undef P5
#undef P6
#undef P7
  };

  tmap.visit_each(visit_fn);

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{GL_LINES, vertices.size(), num_indices, boost::none};
  copy_to_gpu(logger, shader_program, dinfo, vertices, indices);
  return dinfo;
}

WorldOriginArrows
create_axis_arrows(stlw::Logger &logger, ShaderProgram &sp, glm::vec3 const& origin)
{
  auto x = create_arrow(logger, sp, ArrowCreateParams{LOC::RED, origin, origin + X_UNIT_VECTOR});
  auto y = create_arrow(logger, sp, ArrowCreateParams{LOC::GREEN, origin, origin + Y_UNIT_VECTOR});
  auto z = create_arrow(logger, sp, ArrowCreateParams{LOC::BLUE, origin, origin + Z_UNIT_VECTOR});
  return WorldOriginArrows{MOVE(x), MOVE(y), MOVE(z)};
}

WorldOriginArrows
create_world_axis_arrows(stlw::Logger &logger, ShaderProgram &sp)
{
  glm::vec3 constexpr ORIGIN = glm::zero<glm::vec3>();
  return create_axis_arrows(logger, sp, ORIGIN);
}

DrawInfo
copy_gpu(stlw::Logger &logger, GLenum const draw_mode, ShaderProgram &sp, obj const& object,
    boost::optional<TextureInfo> const& ti)
{
  auto const& vertices = object.vertices;
  auto const& indices = object.indices;

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{draw_mode, vertices.size(), num_indices, ti};
  copy_to_gpu(logger, sp, dinfo, vertices, indices);
  return dinfo;
}

} // ns factories::factories
