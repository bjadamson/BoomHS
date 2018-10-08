#pragma once
#include <boomhs/math.hpp>
#include <common/log.hpp>
#include <extlibs/glm.hpp>

#include <array>

namespace boomhs
{
struct CubeTransform;
struct Transform;

struct Ray
{
  glm::vec3 const origin;    // In world space
  glm::vec3 const direction; // In world space. Normalized.
  glm::vec3 const invdir;    // In world space. Normalized.

  std::array<int, 3> const sign;

  explicit Ray(glm::vec3 const&, glm::vec3 const&);
};

} // namespace boomhs

namespace boomhs::collision
{

// Determine whether an axis-aligned Ray and the Cube intersect.
//
// Ray: A ray to cast when determining the intersections.
// Transform: The transform applied to the Cube when applying the intersection test.
// Cube: The cube being tested for intersection witht he ray.
// Float: The output value being the distance the ray traveled before intersecting with the cube.
bool
intersects(common::Logger&, Ray const&, Transform const&, Cube const&, float&);

// Determine whether an axis-aligned Point and the RectFloat intersect.
// Point: A point within a 2-dimensional coordinate system.
// Rect: A rectangle in a 2-dimensionsional coordinate system.
bool
intersects(glm::vec2 const&, RectFloat const&);

// Determine whether two axis-aligned 2-dimensional rectangles overlap.
bool
overlap_axis_aligned(RectFloat const&, RectFloat const&);

// Determine whether two axis-aligned cubes overlap.
bool
overlap_axis_aligned(common::Logger&, CubeTransform const&, CubeTransform const&);

} // namespace boomhs::collision
