#pragma once
#include <boomhs/math.hpp>
#include <common/log.hpp>
#include <extlibs/glm.hpp>

#include <array>
#include <utility>

namespace boomhs
{
struct CubeTransform;
struct RectTransform;
struct Transform;
class  Viewport;

struct Ray
{
  glm::vec3 const origin;    // In world space
  glm::vec3 const direction; // In world space. Normalized.
  glm::vec3 const invdir;    // In world space. Normalized.

  std::array<int, 3> const sign;

  explicit Ray(glm::vec3 const&, glm::vec3 const&);
};

// Oriented Bounding Box
//
// Contains the necessary information to perform a collision detection test in 3 dimensions.
struct OBB
{
  glm::vec3 const                right;
  glm::vec3 const                up;
  glm::vec3 const                forward;
  std::array<glm::vec3, 8> const vertices;

  OBB(glm::vec3 const&, glm::vec3 const&, glm::quat const&);

  // Create an OBB from a Cube and a Transform instance.
  static OBB
  from_cube_transform(Cube const&, Transform const&);
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

// Helper for determining if two rectangles overlap.
// Creates an empty transform for the RectFloat and delegates to another overlap() overload.
bool
overlap(RectFloat const&, RectTransform const&, Viewport const&, bool);

// Determine if two rectangles overlap.
//
// If the rectangles are aligned, it uses a simple test. If there is a rotation, then a test using
// the seperating axis theorem is used.
bool
overlap(RectTransform const&, RectTransform const&);

// Determine if two OBBs (3D cube) overlap.
bool
overlap(OBB const&, OBB const&);

} // namespace boomhs::collision
