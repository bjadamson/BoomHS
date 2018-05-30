#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <stlw/algorithm.hpp>

namespace boomhs
{

Ray::Ray(glm::vec3 const& o, glm::vec3 const& d)
    : orig(o)
    , dir(d)
    , invdir(1.0f / dir)
    , sign(stlw::make_array<int>(invdir.x < 0, invdir.y < 0, invdir.z < 0))
{
}

} // ns boomhs

namespace boomhs::collision
{

// algorithm adopted from:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool
box_intersect(Ray const& r, Transform const& transform, AABoundingBox const& box)
{
  auto const& boxpos = transform.translation;

  glm::vec3 const minpos = box.min * transform.scale;
  glm::vec3 const maxpos = box.max * transform.scale;
  std::array<glm::vec3, 2> const bounds{{minpos + boxpos, maxpos + boxpos}};

  // clang-format off
  float txmin  = (bounds[    r.sign[0]].x - r.orig.x) * r.invdir.x;
  float txmax  = (bounds[1 - r.sign[0]].x - r.orig.x) * r.invdir.x;
  float tymin = (bounds[    r.sign[1]].y - r.orig.y) * r.invdir.y;
  float tymax = (bounds[1 - r.sign[1]].y - r.orig.y) * r.invdir.y;

  if ((txmin > tymax) || (tymin > txmax)) {
    return false;
  }
  if (tymin > txmin) {
    txmin = tymin;
  }
  if (tymax < txmax) {
    txmax = tymax;
  }

  float tzmin = (bounds[    r.sign[2]].z - r.orig.z) * r.invdir.z;
  float tzmax = (bounds[1 - r.sign[2]].z - r.orig.z) * r.invdir.z;
  // clang-format on

  if ((txmin > tzmax) || (tzmin > txmax)) {
    return false;
  }
  // if (tzmin > txmin) {
  // txmin = tzmin;
  //}
  // if (tzmax < txmax) {
  // txmax = tzmax;
  //}
  return true;
}

} // namespace boomhs::collision
