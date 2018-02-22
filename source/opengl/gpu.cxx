#include <opengl/gpu.hpp>
#include <opengl/obj.hpp>

using namespace opengl;

namespace
{

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

} // ns anon

namespace opengl::gpu
{

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
copy_gpu(stlw::Logger &logger, GLenum const draw_mode, ShaderProgram &sp, obj const& object,
    std::optional<TextureInfo> const& ti)
{
  auto const& vertices = object.vertices;
  auto const& indices = object.indices;

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{draw_mode, vertices.size(), num_indices, ti};
  copy_synchronous(logger, sp, dinfo, vertices, indices);
  return dinfo;
}

DrawInfo
copy_rectangle_uvs(stlw::Logger &logger, ShaderProgram const& sp, std::optional<TextureInfo> const& ti)
{
  assert(sp.is_2d);
  auto const v = rectangle_vertices();
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
  auto const& i = RECTANGLE_INDICES;

  DrawInfo dinfo{GL_TRIANGLES, vuvs.size(), i.size(), ti};
  copy_synchronous(logger, sp, dinfo, vuvs, i);
  return dinfo;
}

} // ns opengl::gpu
