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
RectangleFactory::from_vertices_and_uvs(VertexFactory::RectangleVertices const& v, UvFactory::RectangleUvs const& uv)
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

  return RectBuffer{MOVE(vertices)};
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
  return RectLineBuffer{MOVE(vertices)};
}

} // ns factories::factories
