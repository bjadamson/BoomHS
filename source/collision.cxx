#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>

namespace boomhs::collision
{

// algorithm adopted from:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool
box_intersect(Ray const& r, Transform const& transform, AABoundingBox const& box)
{
  auto const& boxpos = transform.translation;

  std::array<glm::vec3, 2> bounds;
  bounds[0] = box.bounds[0] + boxpos;
  bounds[1] = box.bounds[1] + boxpos;

  // clang-format off
  float tmin  = (bounds[    r.sign[0]].x - r.orig.x) * r.invdir.x;
  float tmax  = (bounds[1 - r.sign[0]].x - r.orig.x) * r.invdir.x;
  float tymin = (bounds[    r.sign[1]].y - r.orig.y) * r.invdir.y;
  float tymax = (bounds[1 - r.sign[1]].y - r.orig.y) * r.invdir.y;

  if ((tmin > tymax) || (tymin > tmax)) {
    return false;
  }
  if (tymin > tmin) {
    tmin = tymin;
  }
  if (tymax < tmax) {
    tmax = tymax;
  }

  float tzmin = (bounds[    r.sign[2]].z - r.orig.z) * r.invdir.z;
  float tzmax = (bounds[1 - r.sign[2]].z - r.orig.z) * r.invdir.z;
  // clang-format on

  if ((tmin > tzmax) || (tzmin > tmax)) {
    return false;
  }
  // if (tzmin > tmin) {
  // tmin = tzmin;
  //}
  // if (tzmax < tmax) {
  // tmax = tzmax;
  //}
  return true;
}

} // namespace boomhs::collision
