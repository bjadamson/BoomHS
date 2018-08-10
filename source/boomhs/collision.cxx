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

bool
ray_obb_intersection(
  glm::vec3 const& ray_origin,    // Ray origin, in world space
  glm::vec3 const& ray_direction, // Ray direction (NOT target position!), in world space. Must be normalize()'d.
  glm::vec3 const& aabb_min,      // Minimum X,Y,Z coords of the mesh when not transformed at all.
  glm::vec3 const& aabb_max,      // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's not always the case.
  glm::mat4 const& ModelMatrix,   // Transformation applied to the mesh (which will thus be also applied to its bounding box)
  float& intersection_distance    // Output : distance between ray_origin and the intersection with the OBB
)
{
  // Intersection method from Real-Time Rendering and Essential Mathematics for Games

  float t_min = 0.0f;
  float t_max = 100000.0f;

  glm::vec3 const OBB_position_worldspace(ModelMatrix[3].x, ModelMatrix[3].y, ModelMatrix[3].z);
  glm::vec3 const delta = OBB_position_worldspace - ray_origin;

  // Test intersection with the 2 planes perpendicular to the OBB's X axis
#define TEST_PLANE_INTERSECTION_IMPL(INDEX, AABB_MIN, AABB_MAX)                                    \
  {                                                                                                \
    glm::vec3 axis(ModelMatrix[INDEX].x, ModelMatrix[INDEX].y, ModelMatrix[INDEX].z);              \
    float e = glm::dot(axis, delta);                                                               \
    float f = glm::dot(ray_direction, axis);                                                       \
                                                                                                   \
    if ( fabs(f) > 0.001f ){ /* Standard case */                                                   \
      float t1 = (e + AABB_MIN) / f; /* Intersection with the "left" plane */                      \
      float t2 = (e + AABB_MAX) / f; /* Intersection with the "right" plane */                     \
      /* t1 and t2 now contain distances betwen ray origin and ray-plane intersections */          \
                                                                                                   \
      /* We want t1 to represent the nearest intersection, */                                      \
      /* so if it's not the case, invert t1 and t2*/                                               \
      if (t1 > t2) {                                                                               \
        float w=t1;t1=t2;t2=w; /* swap t1 and t2*/                                                 \
      }                                                                                            \
                                                                                                   \
      /* t_max is the nearest "far" intersection (amongst the X,Y and Z planes pairs) */           \
      if (t2 < t_max) {                                                                            \
        t_max = t2;                                                                                \
      }                                                                                            \
      /* t_min is the farthest "near" intersection (amongst the X,Y and Z planes pairs) */         \
      if (t1 > t_min) {                                                                            \
        t_min = t1;                                                                                \
      }                                                                                            \
                                                                                                   \
      /* And here's the trick : */                                                                 \
      /* If "far" is closer than "near", then there is NO intersection. */                         \
      /* See the images in the tutorials for the visual explanation. */                            \
      if (t_max < t_min) {                                                                         \
        return false;                                                                              \
      }                                                                                            \
    }                                                                                              \
    else if(-e + AABB_MIN > 0.0f || -e + AABB_MAX < 0.0f) {                                        \
      /* Rare case : the ray is almost parallel to the planes, so they don't have any */           \
      /* intersection. */                                                                          \
      return false;                                                                                \
    }                                                                                              \
  }
  TEST_PLANE_INTERSECTION_IMPL(0, aabb_min.x, aabb_max.x);
  TEST_PLANE_INTERSECTION_IMPL(1, aabb_min.y, aabb_max.y);
  TEST_PLANE_INTERSECTION_IMPL(2, aabb_min.z, aabb_max.z);
#undef TEST_PLANE_INTERSECTION_IMPL

  intersection_distance = t_min;
  return true;
}

// algorithm adopted from:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool
ray_box_intersect(Ray const& r, Transform const& transform, AABoundingBox const& box,
    float& distance)
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

  // TODO: I tested whether this was working quickly, it is possible it does not report the correct
  // value in any/all/some cases.
  distance = tzmin;
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
