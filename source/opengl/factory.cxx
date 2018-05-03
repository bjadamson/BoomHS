#include <opengl/factory.hpp>

#include <stlw/algorithm.hpp>
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
ArrayVertices
make_arrow_vertices(ArrowCreateParams const& params)
{
  auto endpoints = calculate_arrow_endpoints(params);
  auto const& p1 = endpoints.p1, p2 = endpoints.p2;
#define COLOR params.color.r(), params.color.g(), params.color.b(), params.color.a()
#define START params.start.x, params.start.y, params.start.z
#define END params.end.x, params.end.y, params.end.z
#define P1 p1.x, p1.y, p1.z
#define P2 p2.x, p2.y, p2.z
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
CubeVertices
cube_vertices()
{
  // Define the 8 vertices of a unit cube
  static const CubeVertices v = stlw::make_array<float>(
    // front
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    // back
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f
  );
  return v;
}

// Rectangles
///////////////////////////////////////////////////////////////////////////////////////////////////
RectangleVertices::RectangleVertices()
{
  stlw::memzero(varray_.data(), sizeof(float) * varray_.size());
}

RectangleVertices::RectangleVertices(VerticesArray const& va)
  : varray_(va)
{
}

RectangleVertices::RectangleVertices(VerticesArray &&va)
  : varray_(MOVE(va))
{
}

RectangleVertices::PointArray
RectangleVertices::zero() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[0], v[1], v[2]);
}

RectangleVertices::PointArray
RectangleVertices::one() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[3], v[4], v[5]);
}

RectangleVertices::PointArray
RectangleVertices::two() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[6], v[7], v[8]);
}

RectangleVertices::PointArray
RectangleVertices::three() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[9], v[10], v[11]);
}

RectangleVertices::PointArray
RectangleVertices::four() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[12], v[13], v[14]);
}

RectangleVertices::PointArray
RectangleVertices::five() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[15], v[16], v[17]);
}

////////////////////////
RectangleUvs::RectangleUvs()
{
  stlw::memzero(varray_.data(), sizeof(float) * varray_.size());
}

RectangleUvs::RectangleUvs(VerticesArray const& va)
  : varray_(va)
{
}

RectangleUvs::RectangleUvs(VerticesArray &&va)
  : varray_(MOVE(va))
{
}

PointArray
RectangleUvs::zero() const
{
  auto const& v = varray_[0];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

PointArray
RectangleUvs::one() const
{
  auto const& v = varray_[1];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

PointArray
RectangleUvs::two() const
{
  auto const& v = varray_[2];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

PointArray
RectangleUvs::three() const
{
  auto const& v = varray_[3];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

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
rectangle_vertices()
{
  float constexpr Z = 0.0f;
#define zero  -1.0f, -1.0f, Z
#define one    1.0f, -1.0f, Z
#define two    1.0f,  1.0f, Z
#define three -1.0f,  1.0f, Z
  return stlw::make_array<float>(
      zero, one, two,
      two, three, zero
      );
#undef zero
#undef one
#undef two
#undef three
}

} // ns factories::factories
