#pragma once
#include <extlibs/glm.hpp>

#include <array>
#include <cmath>
#include <limits>

namespace boomhs
{
struct Transform;

struct Cube
{
  glm::vec3 min, max;

  // methods
  glm::vec3 dimensions() const;
  glm::vec3 center() const;
  glm::vec3 half_widths() const;

  glm::vec3 scaled_min(Transform const&) const;
  glm::vec3 scaled_max(Transform const&) const;

  explicit Cube(glm::vec3 const&, glm::vec3 const&);
};

} // namespace boomhs

namespace boomhs::math
{

template <typename T>
inline auto
squared(T const& value)
{
  return value * value;
}

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

inline glm::vec3
rotate_around(glm::vec3 const& point_to_rotate, glm::vec3 const& rot_center,
              glm::mat4x4 const& rot_matrix)
{
  glm::mat4x4 const translate     = glm::translate(glm::mat4{}, rot_center);
  glm::mat4x4 const inv_translate = glm::translate(glm::mat4{}, -rot_center);

  // The idea:
  // 1) Translate the object to the center
  // 2) Make the rotation
  // 3) Translate the object back to its original location
  glm::mat4x4 const transform = translate * rot_matrix * inv_translate;
  auto const        pos       = transform * glm::vec4{point_to_rotate, 1.0f};
  return glm::vec3{pos.x, pos.y, pos.z};
}

// Original source for algorithm.
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat
rotation_between_vectors(glm::vec3 start, glm::vec3 dest);

// Calculates the cube's dimensions, given a min/max point in 3D.
//
// Interpret the glm::vec3 returned like:
//   x == width
//   y == height
//   z == length
inline glm::vec3
calculate_cube_dimensions(glm::vec3 const& min, glm::vec3 const& max)
{
  auto const w = max.x - min.x;
  auto const h = max.y - min.y;
  auto const l = max.z - min.z;
  return glm::vec3{w, h, l};
}

} // namespace boomhs::math
