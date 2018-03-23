#include <opengl/factory.hpp>
#include <opengl/draw_info.hpp>

#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>
#include <array>

using namespace boomhs;

namespace opengl::factories
{

RectBuffer
create_rectangle(RectInfo const& info)
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
    vertices.emplace_back(0.0);
    vertices.emplace_back(1.0);

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
      vertices.emplace_back(uv.x);
      vertices.emplace_back(uv.y);
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

  auto indices = stlw::make_array<GLuint>(
      0u, 1u, 2u,
      3u, 4u, 5u
      );

  return RectBuffer{MOVE(vertices), MOVE(indices)};
}

} // ns factories::factories
