#include <opengl/factory.hpp>

#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>
#include <array>

using namespace boomhs;
using namespace opengl::factories;

namespace
{

struct ArrowEndpoints
{
  glm::vec3 p1;
  glm::vec3 p2;
};

auto
calculate_arrow_endpoints(OF::ArrowCreateParams const& params)
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

} // ns anon

namespace opengl::factories
{

// Arrows
///////////////////////////////////////////////////////////////////////////////////////////////////
std::array<float, 48>
make_arrow_vertices(ArrowCreateParams const& params)
{
  auto endpoints = calculate_arrow_endpoints(params);
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

// Cubes
///////////////////////////////////////////////////////////////////////////////////////////////////
std::array<float, 32>
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

// Rectangles
///////////////////////////////////////////////////////////////////////////////////////////////////
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

  auto indices = OF::RECTANGLE_INDICES;
  return RectBuffer{MOVE(vertices), MOVE(indices)};
}

std::array<float, 16>
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

} // ns factories::factories
