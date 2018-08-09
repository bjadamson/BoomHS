#pragma once
#include <boomhs/components.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{
class Transform;

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
ray_box_intersect(Ray const&, Transform const&, AABoundingBox const&);

bool
bbox_intersects(stlw::Logger&, Transform const&, AABoundingBox const&, Transform const&,
                AABoundingBox const&);

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
                      // bounding box)
    float&);          // Output : distance between ray_origin and the intersection with the OBB)

} // namespace boomhs::collision
