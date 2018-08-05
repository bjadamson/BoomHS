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

} // namespace boomhs

namespace boomhs::collision
{

// algorithm adopted from:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool
ray_box_intersect(Ray const& r, Transform const& transform, AABoundingBox const& box)
{
  auto const& boxpos = transform.translation;

  glm::vec3 const                minpos = box.min * transform.scale;
  glm::vec3 const                maxpos = box.max * transform.scale;
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

bool
bbox_intersects(stlw::Logger& logger, Transform const& at, AABoundingBox const& ab,
                Transform const& bt, AABoundingBox const& bb)
{
  auto const& ac = at.translation;
  auto const& bc = bt.translation;

  auto const ah = ab.half_widths() * at.scale;
  auto const bh = bb.half_widths() * bt.scale;

  bool const x = std::fabs(ac.x - bc.x) <= (ah.x + bh.x);
  bool const y = std::fabs(ac.y - bc.y) <= (ah.y + bh.y);
  bool const z = std::fabs(ac.z - bc.z) <= (ah.z + bh.z);

  return x && y && z;
}

} // namespace boomhs::collision
