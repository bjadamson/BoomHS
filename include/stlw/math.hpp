#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <array>
#include <utility>

namespace stlw::math
{

// Normalizes "value" from the "from_range" to the "to_range"
template <typename T, typename P1, typename P2>
constexpr float
normalize(T const value, P1 const& from_range, P2 const& to_range)
{
  static_assert(std::is_integral<T>::value, "Input must be integral");
  auto const minimum = from_range.first;
  auto const maximum = from_range.second;
  auto const floor = to_range.first;
  auto const ceil = to_range.second;
  auto const normalized = ((ceil - floor) * (value - minimum))/(maximum - minimum) + floor;
  return static_cast<float>(normalized);
}


inline float
angle_between_vectors(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& origin)
{
  using namespace glm;
  vec3 const da = normalize(a - origin);
  vec3 const db = normalize(b - origin);

  return acos(dot(da, db));
}

// Original source for algorithm.
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
inline glm::quat
rotation_between_vectors(glm::vec3 start, glm::vec3 dest)
{
  using namespace glm;
  start = normalize(start);
  dest = normalize(dest);

  float const cos_theta = dot(start, dest);
  vec3 axis;
  if (cos_theta < -1 + 0.001f) {
    // special case when vectors in opposite directions:
    // there is no "ideal" rotation axis
    // So guess one; any will do as long as it's perpendicular to start
    axis = cross(vec3(0.0f, 0.0f, 1.0f), start);
    if (glm::length2(axis) < 0.01 ) {
      // bad luck, they were parallel, try again!
      axis = cross(vec3(1.0f, 0.0f, 0.0f), start);
    }

    axis = normalize(axis);
    return angleAxis(radians(180.0f), axis);
  }
  axis = cross(start, dest);

  float const s = sqrt( (1 + cos_theta) * 2 );
  float const invs = 1 / s;

  return glm::quat{s * 0.5f,
    axis.x * invs,
    axis.y * invs,
    axis.z * invs};
}

} // ns stlw::math
