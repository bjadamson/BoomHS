#include <opengl/gpu.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/global.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <boomhs/obj.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/types.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/math.hpp>
#include <stlw/optional.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>
#include <array>

using namespace boomhs;
using namespace opengl;
using namespace opengl::gpu;

namespace
{

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
#define COLOR params.color.r(), params.color.g(), params.color.b(), params.color.a()
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

// clang-format off
static constexpr std::array<GLuint, 36> CUBE_INDICES = {{
  0, 1, 2,  2, 3, 0, // front
  1, 5, 6,  6, 2, 1, // top
  7, 6, 5,  5, 4, 7, // back
  4, 0, 3,  3, 7, 4, // bottom
  4, 5, 1,  1, 0, 4, // left
  3, 2, 6,  6, 7, 3, // right
}};

static constexpr std::array<GLuint, 36> CUBE_INDICES_LIGHT = {{
  0, 1, 2, 3, 4, 5, 6,
  7, 8, 9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, 34, 35
}};

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

static constexpr std::array<GLuint, 6> RECTANGLE_INDICES = {{
  0, 1, 2,
  2, 3, 0
}};

auto
rectangle_vertices()
{
  float constexpr W = 1.0f;
  float constexpr Z = 0.0f;
#define zero  -1.0f, -1.0f, Z, W
#define one    1.0f, -1.0f, Z, W
#define two    1.0f,  1.0f, Z, W
#define three -1.0f,  1.0f, Z, W
  return stlw::make_array<float>(
      zero, one, two, three
      );
#undef zero
#undef one
#undef two
#undef three
}

// clang-format on

template<size_t N, size_t M>
DrawInfo
make_drawinfo(stlw::Logger &logger, ShaderProgram const& sp,
    std::array<float, N> const& vertex_data, std::array<GLuint, M> const& indices,
    std::optional<TextureInfo> const& ti)
{
  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{GL_TRIANGLES, vertex_data.size(), num_indices, ti};
  gpu::copy_synchronous(logger, sp, dinfo, vertex_data, indices);
  return dinfo;
}

template<typename V, typename I>
DrawInfo
copy_gpu_impl(stlw::Logger &logger, GLenum const draw_mode, ShaderProgram &sp, V const& vertices,
    I const& indices, std::optional<TextureInfo> const& ti)
{
  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{draw_mode, vertices.size(), num_indices, ti};
  gpu::copy_synchronous(logger, sp, dinfo, vertices, indices);
  return dinfo;
}

} // ns anon

namespace opengl::gpu
{

DrawInfo
create_arrow_2d(stlw::Logger &logger, ShaderProgram const& shader_program, ArrowCreateParams &&params)
{
  auto endpoints = calculate_arrow_endpoints(params);
  auto const vertices = make_arrow_vertices(params, endpoints);

  static constexpr std::array<GLuint, 6> INDICES = {{
    0, 1, 2, 3, 4, 5
  }};

  DrawInfo dinfo{GL_LINES, vertices.size(), INDICES.size(), std::nullopt};
  gpu::copy_synchronous(logger, shader_program, dinfo, vertices, INDICES);
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

  DrawInfo dinfo{GL_LINES, vertices.size(), INDICES.size(), std::nullopt};
  gpu::copy_synchronous(logger, shader_program, dinfo, vertices, INDICES);
  return dinfo;
}

DrawInfo
create_tilegrid(stlw::Logger &logger, ShaderProgram const& shader_program, TileGrid const& tgrid,
    bool const show_yaxis_lines, Color const& color)
{
  std::vector<float> vertices;
  vertices.reserve(tgrid.num_tiles() * 8);

  std::vector<GLuint> indices;
  indices.reserve(tgrid.num_tiles());

  size_t count = 0u;
  auto const add_point = [&indices, &vertices, &count, &color](glm::vec3 const& pos) {
    vertices.emplace_back(pos.x);
    vertices.emplace_back(pos.y);
    vertices.emplace_back(pos.z);
    vertices.emplace_back(1.0f);

    vertices.emplace_back(color.r());
    vertices.emplace_back(color.g());
    vertices.emplace_back(color.b());
    vertices.emplace_back(color.a());

    indices.emplace_back(count++);
  };

  auto const add_line = [&add_point](glm::vec3 const& p0, glm::vec3 const& p1) {
    add_point(p0);
    add_point(p1);
  };

  auto const visit_fn = [&add_line, &show_yaxis_lines](auto const& pos) {
    auto const x = pos.x, y = 0ul, z = pos.y;
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

  visit_each(tgrid, visit_fn);

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{GL_LINES, vertices.size(), num_indices, std::nullopt};
  gpu::copy_synchronous(logger, shader_program, dinfo, vertices, indices);
  return dinfo;
}

WorldOriginArrows
create_axis_arrows(stlw::Logger &logger, ShaderProgram &sp)
{
  glm::vec3 constexpr ORIGIN = glm::zero<glm::vec3>();

  auto x = create_arrow(logger, sp, ArrowCreateParams{LOC::RED,   ORIGIN, ORIGIN + X_UNIT_VECTOR});
  auto y = create_arrow(logger, sp, ArrowCreateParams{LOC::GREEN, ORIGIN, ORIGIN + Y_UNIT_VECTOR});
  auto z = create_arrow(logger, sp, ArrowCreateParams{LOC::BLUE,  ORIGIN, ORIGIN + Z_UNIT_VECTOR});
  return WorldOriginArrows{MOVE(x), MOVE(y), MOVE(z)};
}

DrawInfo
create_modelnormals(stlw::Logger &logger, ShaderProgram const& sp, glm::mat4 const& model_matrix,
    ObjBuffer const& obj, Color const& color)
{
  auto const normal_matrix = glm::inverseTranspose(model_matrix);
  std::vector<float> const& vertices = obj.vertices;

  assert((vertices.size() % 11) == 0);
  std::vector<glm::vec4> positions;
  std::vector<glm::vec3> normals;
  for(auto i = 0u; i < vertices.size(); i += 11) {
    auto const x = vertices[i + 0];
    auto const y = vertices[i + 1];
    auto const z = vertices[i + 2];
    auto const w = 1.0f;

    positions.emplace_back(glm::vec4{x, y, z, w});

    auto const xn = vertices[i + 4];
    auto const yn = vertices[i + 5];
    auto const zn = vertices[i + 6];

    normals.emplace_back(glm::vec3{xn, yn, zn});
  }
  assert(normals.size() == positions.size());

  auto const compute_surfacenormal = [&normal_matrix, &model_matrix](auto const& a_normal) {
    auto const v_normal = normal_matrix * glm::vec4{a_normal, 0.0};
    return glm::normalize(model_matrix * v_normal);
  };

  std::vector<float> line_vertices;
  std::vector<uint32_t> indices;
  FOR(i, normals.size()) {
    line_vertices.emplace_back(positions[i].x);
    line_vertices.emplace_back(positions[i].y);
    line_vertices.emplace_back(positions[i].z);
    line_vertices.emplace_back(positions[i].w);

    line_vertices.emplace_back(LOC::PINK.r());
    line_vertices.emplace_back(LOC::PINK.g());
    line_vertices.emplace_back(LOC::PINK.b());
    line_vertices.emplace_back(LOC::PINK.a());

    auto const surfacenormal = compute_surfacenormal(normals[i]);
    line_vertices.emplace_back(positions[i].x + surfacenormal.x);
    line_vertices.emplace_back(positions[i].y + surfacenormal.y);
    line_vertices.emplace_back(positions[i].z + surfacenormal.z);
    line_vertices.emplace_back(1.0f);

    line_vertices.emplace_back(LOC::PURPLE.r());
    line_vertices.emplace_back(LOC::PURPLE.g());
    line_vertices.emplace_back(LOC::PURPLE.b());
    line_vertices.emplace_back(LOC::PURPLE.a());

    indices.push_back(i);
  }

  DrawInfo dinfo{GL_LINES, vertices.size(), static_cast<GLuint>(indices.size()), std::nullopt};
  gpu::copy_synchronous(logger, sp, dinfo, vertices, indices);
  return dinfo;
}

DrawInfo
copy_colorcube_gpu(stlw::Logger &logger, ShaderProgram const& sp, Color const& color)
{
  // clang-format off
  std::array<Color, 8> const c{
      color, color, color, color,
      color, color, color, color,
  };
#define COLOR(i) c[i].r(), c[i].g(), c[i].b(), c[i].a()
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
  return make_drawinfo(logger, sp, vertex_data, CUBE_INDICES, std::nullopt);
}

DrawInfo
copy_normalcolorcube_gpu(stlw::Logger &logger, ShaderProgram const& sp, Color const& color)
{
  // clang-format off
  static std::array<glm::vec3, 8> constexpr points = {{
    glm::vec3{-0.5f, -0.5f,  0.5f},
    glm::vec3{-0.5f,  0.5f,  0.5f},
    glm::vec3{ 0.5f,  0.5f,  0.5f},
    glm::vec3{ 0.5f, -0.5f,  0.5f},
    glm::vec3{-0.5f, -0.5f, -0.5f},
    glm::vec3{-0.5f,  0.5f, -0.5f},
    glm::vec3{ 0.5f,  0.5f, -0.5f},
    glm::vec3{ 0.5f, -0.5f, -0.5f}
  }};

  std::array<glm::vec3, 36> vertices = {glm::vec3{0.0f}};
  std::array<glm::vec3, 36> normals{glm::vec3{0.0f}};
  std::array<Color, 36> colors{LOC::BLACK};

  auto make_face = [&vertices, &normals, &colors](int const a, int const b, int const c, int const d,
      std::array<glm::vec3, 8> const& points, Color const& face_color, int index)
  {
    using namespace glm;
    vec3 const normal = normalize(cross(points[c] - points[b], points[a] - points[b]));

    vertices[index] = points[a];
    normals[index] = normal;
    colors[index] = face_color;
    index++;

    vertices[index] = points[b];
    normals[index] = normal;
    colors[index] = face_color;
    index++;

    vertices[index] = points[c];
    normals[index] = normal;
    colors[index] = face_color;
    index++;

    vertices[index] = points[a];
    normals[index] = normal;
    colors[index] = face_color;
    index++;

    vertices[index] = points[c];
    normals[index] = normal;
    colors[index] = face_color;
    index++;

    vertices[index] = points[d];
    normals[index] = normal;
    colors[index] = face_color;
  };

  make_face(1, 0, 3, 2, points, color, 6 * 0); // z
  make_face(2, 3, 7, 6, points, color, 6 * 1); // x
  make_face(3, 0, 4, 7, points, color, 6 * 2); // -y
  make_face(6, 5, 1, 2, points, color, 6 * 3); // y
  make_face(4, 5, 6, 7, points, color, 6 * 4); // -z
  make_face(5, 4, 0, 1, points, color, 6 * 5); // -x

  std::vector<float> vertex_data;
  FOR(i, vertices.size()) {
    vertex_data.emplace_back(vertices[i].x);
    vertex_data.emplace_back(vertices[i].y);
    vertex_data.emplace_back(vertices[i].z);
    vertex_data.emplace_back(1.0f);

    vertex_data.emplace_back(normals[i].x);
    vertex_data.emplace_back(normals[i].y);
    vertex_data.emplace_back(normals[i].z);

    vertex_data.emplace_back(colors[i].r());
    vertex_data.emplace_back(colors[i].g());
    vertex_data.emplace_back(colors[i].b());
    vertex_data.emplace_back(colors[i].a());
  }

  auto const& indices = CUBE_INDICES_LIGHT;
  DrawInfo dinfo{GL_TRIANGLES, vertex_data.size(), indices.size(), std::nullopt};
  copy_synchronous(logger, sp, dinfo, vertex_data, indices);
  return dinfo;
}

DrawInfo
copy_vertexonlycube_gpu(stlw::Logger &logger, ShaderProgram const& sp)
{
  auto const vertices = cube_vertices();
  return make_drawinfo(logger, sp, vertices, CUBE_INDICES, std::nullopt);
}

DrawInfo
copy_texturecube_gpu(stlw::Logger &logger, ShaderProgram const& sp, TextureInfo const& ti)
{
  auto const vertices = cube_vertices();
  return make_drawinfo(logger, sp, vertices, CUBE_INDICES, std::make_optional(ti));
}

DrawInfo
copy_cube_14indices_gpu(stlw::Logger &logger, ShaderProgram const& shader_program,
    std::optional<TextureInfo> const& ti)
{
  // clang-format off
  static constexpr std::array<GLuint, 14> INDICES = {{
    3, 2, 6, 7, 4, 2, 0,
    3, 1, 6, 5, 4, 1, 0
  }};

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
  // clang-format on
  auto const& vertices = v;

  DrawInfo dinfo{GL_TRIANGLE_STRIP, vertices.size(), INDICES.size(), ti};
  copy_synchronous(logger, shader_program, dinfo, vertices, INDICES);
  return dinfo;
}

DrawInfo
copy_gpu(stlw::Logger &logger, GLenum const dm, ShaderProgram &sp, ObjBuffer const& object,
    std::optional<TextureInfo> const& ti)
{
  auto const& v = object.vertices;
  auto const& i = object.indices;
  return copy_gpu_impl(logger, dm, sp, v, i, ti);
}

DrawInfo
copy_rectangle(stlw::Logger &logger, GLenum const dm, ShaderProgram &sp,
    OF::RectBuffer const& buffer, std::optional<TextureInfo> const& ti)
{
  auto const& v = buffer.vertices;
  auto const& i = buffer.indices;
  return copy_gpu_impl(logger, dm, sp, v, i, ti);
}

DrawInfo
copy_rectangle_uvs(stlw::Logger &logger, ShaderProgram const& sp, std::optional<TextureInfo> const& ti)
{
  assert(sp.is_2d);
  auto const v = rectangle_vertices();
  // clang-format off
  static auto constexpr uv = stlw::make_array<float>(
      0.0f, 0.0f,
      1.0f, 0.0f,
      1.0f, 1.0f,
      0.0f, 1.0f
      );
  auto const vuvs = stlw::make_array<float>(
      v[0],  v[1],  v[2],  v[3],  uv[0], uv[1],
      v[4],  v[5],  v[6],  v[7],  uv[2], uv[3],
      v[8],  v[9],  v[10], v[11], uv[4], uv[5],
      v[12], v[13], v[14], v[15], uv[6], uv[7]
      );
  // clang-format on
  auto const& i = RECTANGLE_INDICES;

  DrawInfo dinfo{GL_TRIANGLES, vuvs.size(), i.size(), ti};
  copy_synchronous(logger, sp, dinfo, vuvs, i);
  return dinfo;
}

} // ns opengl::gpu
