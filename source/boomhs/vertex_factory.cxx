#include <boomhs/vertex_factory.hpp>
#include <boomhs/math.hpp>

using namespace boomhs;

namespace
{

struct ArrowEndpoints
{
  glm::vec3 start;
  glm::vec3 end;
};

auto
calculate_arrow_endpoints(ArrowTemplate const& arrow)
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
  auto const A = adjust_if_zero(arrow.start);
  auto const B = adjust_if_zero(arrow.end);

  glm::vec3 const v = A - B;
  glm::vec3 const rev = -v;

  glm::vec3 const cross1 = glm::normalize(glm::cross(A, B));
  glm::vec3 const cross2 = glm::normalize(glm::cross(B, A));

  glm::vec3 const vp1 = glm::normalize(rev + cross1);
  glm::vec3 const vp2 = glm::normalize(rev + cross2) ;

  float const factor = arrow.tip_length_factor;
  glm::vec3 const p1 = B - (vp1 / factor);
  glm::vec3 const p2 = B - (vp2 / factor);

  return ArrowEndpoints{p1, p2};
}

} // ns anon

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Arrow
VertexFactory::ArrowVertices
VertexFactory::build(ArrowTemplate const& arrow)
{
  auto endpoints = calculate_arrow_endpoints(arrow);
  auto const& p1 = endpoints.start, p2 = endpoints.end;

  auto const& color = arrow.color;
  auto const& start = arrow.start;
  auto const& end   = arrow.end;
#define COLOR color.r(), color.g(), color.b(), color.a()
#define START start.x,   start.y,   start.z
#define END   end.x,     end.y,     end.z

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

} // namespace boomhs
