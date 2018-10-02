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



// Determine whether a ray and an Axis Aligned Cube
// algorithm adopted from:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-cube-intersection
bool
ray_axis_aligned_cube_intersect(Ray const&, Transform const&, Cube const&, float&);

bool
cubes_overlap(common::Logger&, CubeTransform const&, CubeTransform const&);

// algorithm adapted from:
// http://www.opengl-tutorial.org/kr/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
// https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_custom.cpp
bool
ray_obb_intersection(
    Ray const&,
    glm::vec3 const&, // Minimum X,Y,Z coords of the mesh when not transformed at all.
    glm::vec3 const&, // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's
                      // not always the case.
    glm::mat4 const&, // Transformation applied to the mesh (which will thus be also applied to its
    float&);          // Output : distance between ray_origin and the intersection with the OBB)

bool
ray_obb_intersection(Ray const&,
                     Cube,             // Bounding cube of the entity we are testing against.
                     Transform,        // Transform
                     float&);          // Output: distance

bool
ray_intersects_cube(common::Logger&, Ray const&, Transform const&, Cube const&, float&);

bool
point_rectangle_intersects(glm::vec2 const&, RectFloat const&);

bool
rectangles_overlap(RectFloat const&, RectFloat const&);

} // namespace boomhs::collision
