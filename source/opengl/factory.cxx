#include <opengl/factory.hpp>
#include <boomhs/viewport.hpp>

#include <common/algorithm.hpp>
#include <boomhs/math.hpp>
#include <common/type_macros.hpp>
#include <array>

using namespace boomhs;
using namespace opengl;
using namespace opengl::factories;

namespace opengl
{

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

} // namespace opengl

namespace opengl::factories
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Cubes
CubeVertices
cube_vertices(glm::vec3 const& min, glm::vec3 const& max)
{
  auto const dimensions = math::compute_cube_dimensions(min, max);
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
RectBuilder::RectBuilder(RectFloat const& r)
    : rect(r)
{
}

RectBuffer
RectBuilder::build() const
{
  // clang-format off
  bool const uniform_only = (uniform_color && (!color_array   && !uvs));
  bool const color_only   = (color_array   && (!uniform_color && !uvs));
  bool const uvs_only     = (uvs           && (!color_array   && !uniform_color));

  bool const none         = !uniform_only && !color_only && !uvs;
  bool const exactly_one  = uniform_only || color_only  || uvs_only;
  assert(exactly_one || none);
  // clang-format on

  return make_rectangle(*this);
}

RectBuffer
make_rectangle(RectBuilder const& builder)
{
  auto const& color_o  = builder.uniform_color;
  auto const& colors_o = builder.color_array;
  auto const& uvs_o    = builder.uvs;

  std::vector<float> vertices;
  auto const add_point = [&](glm::vec2 const& point, int const index)
  {
    assert(index < RectangleColorArray::NUM_VERTICES);
    vertices.emplace_back(point.x);
    vertices.emplace_back(point.y);

    static constexpr float Z = 0.0f;
    vertices.emplace_back(Z);

    if (color_o) {
      auto const& c = *color_o;
      vertices.emplace_back(c.r());
      vertices.emplace_back(c.g());
      vertices.emplace_back(c.b());
      vertices.emplace_back(c.a());
    }
    else if (colors_o) {
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

  auto const& r = builder.rect;
  auto const p0 = glm::vec2{r.left, r.bottom};
  auto const p1 = glm::vec2{r.right, r.bottom};

  auto const p2 = glm::vec2{r.right, r.top};
  auto const p3 = glm::vec2{r.left, r.top};

  add_point(p0, 0);
  add_point(p1, 1);
  add_point(p2, 2);

  add_point(p2, 2);
  add_point(p3, 3);
  add_point(p0, 0);

  auto indices = OF::RECTANGLE_INDICES;
  return RectBuffer{MOVE(vertices), MOVE(indices)};
}

RectLineBuffer
make_line_rectangle(RectFloat const& r)
{
  std::vector<float> vertices;
  auto const add_point = [&vertices](auto const& p) {
    vertices.emplace_back(p.x);
    vertices.emplace_back(p.y);

    static constexpr float Z = 0.0f;
    vertices.emplace_back(Z);
  };

  auto const p0 = glm::vec2{r.left, r.bottom};
  auto const p1 = glm::vec2{r.right, r.bottom};

  auto const p2 = glm::vec2{r.right, r.top};
  auto const p3 = glm::vec2{r.left, r.top};

  add_point(p0);
  add_point(p1);
  add_point(p2);
  add_point(p3);
  RectangleLineIndices indices = {{
    0, 1, 2, 3
  }};
  return RectLineBuffer{MOVE(vertices), MOVE(indices)};
}


RectangleVertices
rectangle_vertices(float const x, float const y, float const w, float const h)
{
  auto const x0 = x;
  auto const y0 = y;

  auto const x1 = x0 + w;
  auto const y1 = y0 - h;

  auto constexpr Z = -0.01f;
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
  using PointArray = RectangleUvs::PointArray;
  return common::make_array<PointArray>(
      PointArray{0.0f, 0.0f},
      PointArray{max, 0.0f},
      PointArray{max, max},
      PointArray{0.0f, max}
      );
}

} // ns factories::factories
