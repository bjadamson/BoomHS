#include <opengl/gpu.hpp>
#include <opengl/factory.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/global.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/obj.hpp>
#include <boomhs/obj_store.hpp>
#include <boomhs/terrain.hpp>


#include <stlw/algorithm.hpp>
#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>
#include <array>

using namespace boomhs;
using namespace opengl;
using namespace opengl::gpu;
using namespace glm;

namespace
{

template <typename VERTICES, typename INDICES>
void
copy_synchronous(stlw::Logger& logger, VertexAttribute const& va, DrawInfo &dinfo,
                 VERTICES const& vertices, INDICES const& indices)
{
  auto const bind_and_copy = [&]() {
    glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinfo.ebo());

    va.upload_vertex_format_to_glbound_vao(logger);

    // copy the vertices
    LOG_DEBUG_SPRINTF("inserting '%i' vertices into GL_BUFFER_ARRAY\n", vertices.size());
    auto const  vertices_size = vertices.size() * sizeof(GLfloat);
    auto const& vertices_data = vertices.data();
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

    // copy the vertice rendering order
    LOG_DEBUG_SPRINTF("inserting '%i' indices into GL_ELEMENT_BUFFER_ARRAY\n", indices.size());
    auto const  indices_size = sizeof(GLuint) * indices.size();
    auto const& indices_data = indices.data();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);
  };

  LOG_TRACE("Starting synchronous cpu -> gpu copy");
  auto &vao = dinfo.vao();
  vao.while_bound(logger, bind_and_copy);
  LOG_TRACE("cpu -> gpu copy complete");
}

template<typename V, typename I>
DrawInfo
copy_gpu_impl(stlw::Logger &logger, VertexAttribute const& va,
    V const& vertices, I const& indices)
{
  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{vertices.size(), num_indices};
  copy_synchronous(logger, va, dinfo, vertices, indices);
  return dinfo;
}

template <size_t N, size_t M>
DrawInfo
make_drawinfo(stlw::Logger &logger, VertexAttribute const& va,
    std::array<float, N> const& vertex_data, std::array<GLuint, M> const& indices)
{
  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{vertex_data.size(), num_indices};
  copy_synchronous(logger, va, dinfo, vertex_data, indices);
  return dinfo;
}

} // ns anon

namespace opengl::gpu
{

DrawInfo
create_arrow_2d(stlw::Logger &logger, VertexAttribute const& va,
    ArrowCreateParams &&params)
{
  auto const vertices = OF::make_arrow_vertices(params);

  static constexpr std::array<GLuint, 6> INDICES = {{
    0, 1, 2, 3, 4, 5
  }};

  DrawInfo dinfo{vertices.size(), INDICES.size()};
  copy_synchronous(logger, va, dinfo, vertices, INDICES);
  return dinfo;
}

DrawInfo
create_arrow(stlw::Logger &logger, VertexAttribute const& va,
    ArrowCreateParams &&params)
{
  auto const vertices = OF::make_arrow_vertices(params);

  static constexpr std::array<GLuint, 6> INDICES = {{
    0, 1, 2, 3, 4, 5
  }};

  DrawInfo dinfo{vertices.size(), INDICES.size()};
  copy_synchronous(logger, va, dinfo, vertices, INDICES);
  return dinfo;
}

DrawInfo
create_terrain_grid(stlw::Logger &logger, VertexAttribute const& va, glm::vec2 const& dimensions,
                bool const show_yaxis_lines, Color const& color)
{
  std::vector<float> vertices;

  std::vector<GLuint> indices;

  size_t count = 0u;
  auto const add_point = [&indices, &vertices, &count, &color](glm::vec3 const& pos) {
    vertices.emplace_back(pos.x);
    vertices.emplace_back(pos.y);
    vertices.emplace_back(pos.z);

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
    auto const x = pos.x, y = 0.0f, z = pos.y;
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

  FOR(x, dimensions.x) {
    FOR(y, dimensions.y) {
      visit_fn(glm::vec2{x, y});
    }
  }

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{vertices.size(), num_indices};
  copy_synchronous(logger, va, dinfo, vertices, indices);
  return dinfo;
}

WorldOriginArrows
create_axis_arrows(stlw::Logger &logger, VertexAttribute const& va)
{
  glm::vec3 constexpr ORIGIN = glm::zero<glm::vec3>();

  auto x = create_arrow(logger, va, ArrowCreateParams{LOC::RED,   ORIGIN, ORIGIN + X_UNIT_VECTOR});
  auto y = create_arrow(logger, va, ArrowCreateParams{LOC::GREEN, ORIGIN, ORIGIN + Y_UNIT_VECTOR});
  auto z = create_arrow(logger, va, ArrowCreateParams{LOC::BLUE,  ORIGIN, ORIGIN + Z_UNIT_VECTOR});
  return WorldOriginArrows{MOVE(x), MOVE(y), MOVE(z)};
}

/*
DrawInfo
copy_cubecolor_gpu(stlw::Logger &logger, CubeMinMax const& cr, VertexAttribute const& va,
    Color const& color)
{
  // clang-format off
  std::array<Color, 8> const c{
      color, color, color, color,
      color, color, color, color,
  };
#define COLOR(i) c[i].r(), c[i].g(), c[i].b(), c[i].a()
#define VERTS(a, b, c, d) v[a], v[b], v[c], v[d]
  auto const v = OF::cube_vertices(cr.min, cr.max);
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
  return make_drawinfo(logger, va, vertex_data, OF::CUBE_INDICES);
}
*/

/*
DrawInfo
copy_cubenormalcolor_gpu(stlw::Logger &logger, CubeMinMax const& cr, VertexAttribute const& va,
    Color const& color)
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

    vertex_data.emplace_back(normals[i].x);
    vertex_data.emplace_back(normals[i].y);
    vertex_data.emplace_back(normals[i].z);

    vertex_data.emplace_back(colors[i].r());
    vertex_data.emplace_back(colors[i].g());
    vertex_data.emplace_back(colors[i].b());
    vertex_data.emplace_back(colors[i].a());
  }

  auto const& indices = OF::CUBE_INDICES_LIGHT;
  DrawInfo dinfo{vertex_data.size(), indices.size()};
  copy_synchronous(logger, va, dinfo, vertex_data, indices);
  return dinfo;
}
*/

DrawInfo
copy_cube_gpu(stlw::Logger &logger, CubeVertices const& cv, VertexAttribute const& va)
{
  return make_drawinfo(logger, va, cv, OF::CUBE_INDICES);
}

DrawInfo
copy_cube_wireframe_gpu(stlw::Logger& logger, CubeVertices const& cv,
    VertexAttribute const& va)
{
  return make_drawinfo(logger, va, cv, OF::CUBE_WIREFRAME_INDICES);
}

DrawInfo
copy_gpu(stlw::Logger &logger, VertexAttribute const& va,
    ObjData const& data)
{
  auto const qa    = BufferFlags::from_va(va);
  auto interleaved = VertexBuffer::create_interleaved(logger, data, qa);

  return copy_gpu_impl(logger, va, interleaved.vertices, interleaved.indices);
}

DrawInfo
copy_gpu(stlw::Logger &logger, VertexAttribute const& va, VertexBuffer const& object)
{
  auto const& v = object.vertices;
  auto const& i = object.indices;
  return copy_gpu_impl(logger, va, v, i);
}

DrawInfo
copy_rectangle(stlw::Logger &logger, VertexAttribute const& va,
    RectBuffer const& buffer)
{
  auto const& v = buffer.vertices;
  auto const& i = buffer.indices;
  return copy_gpu_impl(logger, va, v, i);
}

DrawInfo
copy_rectangle_uvs(stlw::Logger &logger, VertexAttribute const& va,
                   RectangleVertices const& v, TextureInfo const& tinfo)
{
  auto const& i = OF::RECTANGLE_INDICES;
  auto const uv = OF::rectangle_uvs(tinfo.uv_max);

  // clang-format off
  auto const vertices = stlw::concat(
      v.zero(), uv.zero(),
      v.one(),  uv.one(),
      v.two(),  uv.two(),

      v.three(), uv.two(),
      v.four(),  uv.three(),
      v.five(),  uv.zero()
      );
  // clang-format on

  DrawInfo dinfo{vertices.size(), i.size()};
  copy_synchronous(logger, va, dinfo, vertices, i);
  return dinfo;
}

void
overwrite_vertex_buffer(stlw::Logger& logger, VertexAttribute const& va, DrawInfo &dinfo,
                 ObjData const& objdata)
{
  auto const upload = [&]() {
    glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
    va.upload_vertex_format_to_glbound_vao(logger);

    auto const qa    = BufferFlags::from_va(va);
    auto interleaved = VertexBuffer::create_interleaved(logger, objdata, qa);

    auto const& vertices      = interleaved.vertices;
    auto const  vertices_size = vertices.size() * sizeof(GLfloat);
    auto const& vertices_data = vertices.data();
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_size, vertices_data);
  };

  auto &vao = dinfo.vao();
  vao.while_bound(logger, upload);
}

} // ns opengl::gpu
