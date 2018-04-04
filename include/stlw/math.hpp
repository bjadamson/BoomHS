#pragma once
#include <extlibs/glm.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <ostream>
#include <utility>

namespace stlw::math
{

inline bool
opposite_signs(int const x, int const y)
{
  return ((x ^ y) < 0);
}

inline bool
float_compare(float const a, float const b)
{
  return std::fabs(a - b) < std::numeric_limits<float>::epsilon();
}

inline glm::vec3
lerp(glm::vec3 const& a, glm::vec3 const& b, float const f)
{
  return (a * (1.0 - f)) + (b * f);
}

inline glm::mat4
calculate_modelmatrix(glm::vec3 const& translation, glm::quat const& rotation,
                      glm::vec3 const& scale)
{
  auto const tmatrix = glm::translate(glm::mat4{}, translation);
  auto const rmatrix = glm::toMat4(rotation);
  auto const smatrix = glm::scale(glm::mat4{}, scale);
  return tmatrix * rmatrix * smatrix;
}

inline bool
allnan(glm::vec3 const& v)
{
  return std::isnan(v.x) && std::isnan(v.y) && std::isnan(v.z);
}

inline bool
anynan(glm::vec3 const& v)
{
  return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
}

// Normalizes "value" from the "from_range" to the "to_range"
template <typename T, typename P1, typename P2>
constexpr float
normalize(T const value, P1 const& from_range, P2 const& to_range)
{
  static_assert(std::is_integral<T>::value, "Input must be integral");
  auto const minimum    = from_range.first;
  auto const maximum    = from_range.second;
  auto const floor      = to_range.first;
  auto const ceil       = to_range.second;
  auto const normalized = ((ceil - floor) * (value - minimum)) / (maximum - minimum) + floor;
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
  dest  = normalize(dest);

  float const cos_theta = dot(start, dest);
  vec3        axis;
  if (cos_theta < -1 + 0.001f) {
    // special case when vectors in opposite directions:
    // there is no "ideal" rotation axis
    // So guess one; any will do as long as it's perpendicular to start
    axis = cross(vec3(0.0f, 0.0f, 1.0f), start);
    if (glm::length2(axis) < 0.01) {
      // bad luck, they were parallel, try again!
      axis = cross(vec3(1.0f, 0.0f, 0.0f), start);
    }

    axis = normalize(axis);
    return angleAxis(radians(180.0f), axis);
  }
  axis = cross(start, dest);

  float const s    = sqrt((1 + cos_theta) * 2);
  float const invs = 1 / s;

  return glm::quat{s * 0.5f, axis.x * invs, axis.y * invs, axis.z * invs};
}

} // namespace stlw::math

namespace glm
{

inline std::ostream&
operator<<(std::ostream& stream, glm::vec2 const& vec)
{
  stream << to_string(vec);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::vec3 const& vec)
{
  stream << to_string(vec);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::vec4 const& vec)
{
  stream << to_string(vec);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::mat2 const& mat)
{
  stream << to_string(mat);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::mat3 const& mat)
{
  stream << to_string(mat);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::mat4 const& mat)
{
  stream << to_string(mat);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::quat const& quat)
{
  stream << to_string(quat);
  return stream;
}

} // namespace glm
