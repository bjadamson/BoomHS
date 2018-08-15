#include <opengl/factory.hpp>
#include <boomhs/screen_size.hpp>

#include <common/algorithm.hpp>
#include <boomhs/math.hpp>
#include <common/type_macros.hpp>
#include <array>

using namespace boomhs;
using namespace opengl;
using namespace opengl::factories;

namespace
{

struct ArrowEndpoints
{
  glm::vec3 p1;
  glm::vec3 p2;
};

auto
calculate_arrow_endpoints(ArrowCreateParams const& params)
{
  auto const adjust_if_zero = [=](glm::vec3 const& v) {
    auto constexpr EPSILON = std::numeric_limits<float>::epsilon();
    auto constexpr EPSILON_VEC = glm::vec3{EPSILON, EPSILON, EPSILON};
    bool const is_zero = glm::all(glm::epsilonEqual(v, math::constants::ZERO, EPSILON));
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

} // ns anon

namespace opengl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Arrow
ArrowVertices
ArrowFactory::create_vertices(ArrowCreateParams const& params)
{
  auto endpoints = calculate_arrow_endpoints(params);
  auto const& p1 = endpoints.p1, p2 = endpoints.p2;
#define COLOR params.color.r(), params.color.g(), params.color.b(), params.color.a()
#define START params.start.x, params.start.y, params.start.z
#define END params.end.x, params.end.y, params.end.z
#define P1 p1.x, p1.y, p1.z
#define P2 p2.x, p2.y, p2.z
  return common::make_array<float>(
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Rectangle
RectangleUvVertices
RectangleFactory::from_vertices_and_uvs(RectangleVertices const& v, RectangleUvs const& uv)
{
  return common::concat(
    v.zero(), uv.zero(),
    v.one(),  uv.one(),
    v.two(),  uv.two(),

    v.three(), uv.two(),
    v.four(),  uv.three(),
    v.five(),  uv.zero()
  );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Grid
GridVerticesIndices
GridFactory::create_grid(glm::vec2 const& dimensions, bool const show_yaxis_lines,
                         Color const& color)
{
  GridVerticesIndices result;
  auto& indices  = result.indices;
  auto& vertices = result.vertices;

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

  return result;
}

} // namespace opengl

namespace opengl::factories
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Cubes
CubeVertices
cube_vertices(glm::vec3 const& min, glm::vec3 const& max)
{
  auto const dimensions = math::calculate_cube_dimensions(min, max);
  auto const& width     = dimensions.x;
  auto const& height    = dimensions.y;

  return common::make_array<float>(
    min.x,         min.y,          min.z,
    min.x + width, min.y,          min.z,
    min.x + width, min.y + height, min.z,
    min.x,         min.y + height, min.z,

    max.x - width, max.y - height, max.z,
    max.x,         max.y - height, max.z,
    max.x,         max.y,          max.z,
    max.x - width, max.y,          max.z
    );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Rectangles
RectBuffer
make_rectangle(RectInfo const& info)
{
  auto const& color_o = info.color;
  auto const& colors_o = info.colors;

  auto const& uvs_o = info.uvs;

  float const height = info.height;
  float const width = info.width;

  std::vector<float> vertices;
  auto const add_point = [&](glm::vec2 const& point, int const index)
  {
    assert(index < RectInfo::NUM_VERTICES);
    vertices.emplace_back(point.x);
    vertices.emplace_back(point.y);

    static constexpr float Z = 0.0f;
    vertices.emplace_back(Z);

    if (color_o) {
      assert(!colors_o);
      auto const& c = *color_o;
      vertices.emplace_back(c.r());
      vertices.emplace_back(c.g());
      vertices.emplace_back(c.b());
      vertices.emplace_back(c.a());
    }
    else if (colors_o) {
      assert(!color_o);
      auto const& c = (*colors_o)[index];
      vertices.emplace_back(c.r());
      vertices.emplace_back(c.g());
      vertices.emplace_back(c.b());
      vertices.emplace_back(c.a());
    }
    if (uvs_o) {
      auto const& uv = (*uvs_o)[index];
      vertices.emplace_back(uv[0]);
      vertices.emplace_back(uv[1]);
    }
  };

  glm::vec2 constexpr ORIGIN{0, 0};
  auto const p0 = glm::vec2{ORIGIN.x - width, ORIGIN.y - height};
  auto const p1 = glm::vec2{ORIGIN.x + width, ORIGIN.y - height};

  auto const p2 = glm::vec2{ORIGIN.x + width, ORIGIN.y + height};
  auto const p3 = glm::vec2{ORIGIN.x - width, ORIGIN.y + height};

  add_point(p0, 0);
  add_point(p1, 1);
  add_point(p2, 2);

  add_point(p2, 2);
  add_point(p3, 3);
  add_point(p0, 0);

  auto indices = OF::RECTANGLE_INDICES;
  return RectBuffer{MOVE(vertices), MOVE(indices)};
}


RectangleVertices
rectangle_vertices(float const x, float const y, float const w, float const h)
{
  auto const x0 = x;
  auto const y0 = y;

  auto const x1 = x0 + w;
  auto const y1 = y0 - h;

  auto constexpr Z = 0.0f;
#define zero  x0, y0, Z
#define one   x1, y0, Z
#define two   x1, y1, Z
#define three x0, y1, Z
  return common::make_array<float>(
      zero, one, two,
      two, three, zero
      );
#undef zero
#undef one
#undef two
#undef three
}


RectangleVertices
rectangle_vertices_default()
{
  float constexpr Z = 0.0f;
#define zero  -1.0f, -1.0f, Z
#define one    1.0f, -1.0f, Z
#define two    1.0f,  1.0f, Z
#define three -1.0f,  1.0f, Z
  return common::make_array<float>(
      zero, one, two,
      two, three, zero
      );
#undef zero
#undef one
#undef two
#undef three
}

RectangleUvs
rectangle_uvs(float const max)
{
  return common::make_array<PointArray>(
      PointArray{0.0f, 0.0f},
      PointArray{max, 0.0f},
      PointArray{max, max},
      PointArray{0.0f, max}
      );
}

} // ns factories::factories
