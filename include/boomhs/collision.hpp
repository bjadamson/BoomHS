#pragma once
#include <boomhs/math.hpp>
#include <common/log.hpp>
#include <extlibs/glm.hpp>

#include <array>

namespace boomhs
{
struct Transform;

struct Ray
{
  glm::vec3 const& orig;
  glm::vec3 const& dir;
  glm::vec3 const  invdir;

  std::array<int, 3> const sign;

  explicit Ray(glm::vec3 const&, glm::vec3 const&);
};

} // namespace boomhs

namespace boomhs::collision
{

bool
ray_cube_intersect(Ray const&, Transform const&, Cube const&, float&);

bool
cube_intersects(common::Logger&, Transform const&, Cube const&, Transform const&, Cube const&);

// algorithm adapted from:
// http://www.opengl-tutorial.org/kr/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
// https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_custom.cpp
bool
ray_obb_intersection(
    glm::vec3 const&, // Ray origin, in world space
    glm::vec3 const&, // Ray direction (NOT target position!), in world space. Must be
                      // normalize()'d.
    glm::vec3 const&, // Minimum X,Y,Z coords of the mesh when not transformed at all.
    glm::vec3 const&, // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's
                      // not always the case.
    glm::mat4 const&, // Transformation applied to the mesh (which will thus be also applied to its
    float&);          // Output : distance between ray_origin and the intersection with the OBB)

bool
ray_obb_intersection(glm::vec3 const&, // Ray origin
                     glm::vec3 const&, // Ray direction
                     Cube,             // Bounding cube of the entity we are testing against.
                     Transform,        // Transform
                     float&);          // Output: distance

bool
point_rectangle_intersects(glm::vec2 const&, Rectangle const&);

} // namespace boomhs::collision
