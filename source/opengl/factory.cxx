#include <opengl/factory.hpp>
#include <opengl/draw_info.hpp>
#include <extlibs/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/gpu.hpp>
#include <boomhs/obj.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/types.hpp>

#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>

#include <stlw/math.hpp>
#include <stlw/optional.hpp>
#include <array>

using namespace boomhs;

namespace opengl::factories
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

} // ns factories::factories
