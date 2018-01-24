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

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/vector_query.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/string_cast.hpp>

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
  return make_cube_drawinfo(logger, vertex_data, shader_program, boost::none);
}

/*
Vec3f RibbonMesh::calcNormal( const Vec3f &p1, const Vec3f &p2, const Vec3f &p3 )
{
    Vec3f V1= (p2 - p1);
    Vec3f V2 = (p3 - p1);
    Vec3f surfaceNormal;
    surfaceNormal.x = (V1.y*V2.z) - (V1.z-V2.y);
    surfaceNormal.y = - ( (V2.z * V1.x) - (V2.x * V1.z) );
    surfaceNormal.z = (V1.x-V2.y) - (V1.y-V2.x);

    // Dont forget to normalize if needed
    return surfaceNormal;
}
*/

using Face = std::array<float, 6 * (4 + 3 + 4)>;



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

  //#define VERTEX(p) points[p].x,    points[p].y,    points[p].z, 1.0
  //#define NORMAL(p) normal.x,       normal.y,       normal.z
  //#define COLOR(p)  face_color.r(), face_color.g(), face_color.b(), face_color.a()
  //#define FACE(p) VERTEX(p), NORMAL(p), COLOR(p)

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

    //return Face{FACE(a), FACE(b), FACE(c), FACE(a), FACE(c), FACE(d)};
  //#undef VERTEX
  //#undef NORMAL
  //#undef COLOR
  //#undef FACE
  };

  make_face(1, 0, 3, 2, points, color, 6 * 0); // z
  make_face(2, 3, 7, 6, points, color, 6 * 1); // x
  make_face(3, 0, 4, 7, points, color, 6 * 2); // -y
  make_face(6, 5, 1, 2, points, color, 6 * 3); // y
  make_face(4, 5, 6, 7, points, color, 6 * 4); // -z
  make_face(5, 4, 0, 1, points, color, 6 * 5); // -x

#define FACE_VERTEX(v) vertices[v], vertices[v+1], vertices[v+2], 1.0f, \
  normals[v], normals[v+1], normals[v+2], \
  color.r(), color.g(), color.b(), color.a()

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

  auto const& indices = cube_factory::INDICES_LIGHT;
  DrawInfo dinfo{GL_TRIANGLES, vertex_data.size(), indices.size(), boost::none};
  copy_to_gpu(logger, sp, dinfo, vertex_data, indices);
  return dinfo;
}

DrawInfo
copy_vertexonlycube_gpu(stlw::Logger &logger, ShaderProgram const& shader_program)
{
  auto const vertices = cube_vertices();
  return make_cube_drawinfo(logger, vertices, shader_program, boost::none);
}

DrawInfo
copy_texturecube_gpu(stlw::Logger &logger, ShaderProgram const& shader_program, TextureInfo const& ti)
{
  auto const vertices = cube_vertices();
  return make_cube_drawinfo(logger, vertices, shader_program, boost::make_optional(ti));
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

DrawInfo
create_arrow_2d(stlw::Logger &logger, ShaderProgram const& shader_program, ArrowCreateParams &&params)
{
  auto endpoints = calculate_arrow_endpoints(params);
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
  vertices.reserve(tmap.num_tiles() * 8);

  std::vector<GLuint> indices;
  indices.reserve(tmap.num_tiles());

  std::size_t count = 0u;
  auto const add_point = [&indices, &vertices, &count, &color](glm::vec3 const& point) {
    vertices.emplace_back(point.x);
    vertices.emplace_back(point.y);
    vertices.emplace_back(point.z);
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
    obj const& obj, Color const& color)
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

  DrawInfo dinfo{GL_LINES, vertices.size(), static_cast<GLuint>(indices.size()), boost::none};
  copy_to_gpu(logger, sp, dinfo, vertices, indices);
  return dinfo;
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
