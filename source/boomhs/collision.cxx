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

bool
ray_obb_intersection(
  glm::vec3 const& ray_origin,    // Ray origin, in world space
  glm::vec3 const& ray_direction, // Ray direction (NOT target position!), in world space. Must be normalize()'d.
  glm::vec3 const& aabb_min,      // Minimum X,Y,Z coords of the mesh when not transformed at all.
  glm::vec3 const& aabb_max,      // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's not always the case.
  glm::mat4 const& model_matrix,   // Transformation applied to the mesh (which will thus be also applied to its bounding box)
  float& intersection_distance)   // Output : distance between ray_origin and the intersection with the OBB
{
  float t_min = 0.0f;
  float t_max = 100000.0f;

  glm::vec3 const OBB_position_worldspace(model_matrix[3].x, model_matrix[3].y, model_matrix[3].z);
  glm::vec3 const delta = OBB_position_worldspace - ray_origin;

  // Test intersection with the 2 planes perpendicular to the OBB's X axis
  {
    glm::vec3 const xaxis(model_matrix[0].x, model_matrix[0].y, model_matrix[0].z);
    float const e = glm::dot(xaxis, delta);
    float const f = glm::dot(ray_direction, xaxis);

    if (fabs(f) > 0.001f ) { // Standard case
      float t1 = (e+aabb_min.x)/f; // Intersection with the "left" plane
      float t2 = (e+aabb_max.x)/f; // Intersection with the "right" plane
      // t1 and t2 now contain distances betwen ray origin and ray-plane intersections

      // We want t1 to represent the nearest intersection, 
      // so if it's not the case, invert t1 and t2
      if (t1>t2) {
        // swap t1 and t2
        float const w = t1;
        t1 = t2;
        t2 = w;
      }

      // t_max is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
      if ( t2 < t_max )
        t_max = t2;
      // t_min is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
      if ( t1 > t_min )
        t_min = t1;

      // And here's the trick :
      // If "far" is closer than "near", then there is NO intersection.
      // See the images in the tutorials for the visual explanation.
      if (t_max < t_min )
        return false;
    }
    else { // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
    if(-e+aabb_min.x > 0.0f || -e+aabb_max.x < 0.0f)
      return false;
    }
  }


  // Test intersection with the 2 planes perpendicular to the OBB's Y axis
  // Exactly the same thing than above.
  {
    glm::vec3 yaxis(model_matrix[1].x, model_matrix[1].y, model_matrix[1].z);
    float e = glm::dot(yaxis, delta);
    float f = glm::dot(ray_direction, yaxis);

    if ( fabs(f) > 0.001f ) {
      float t1 = (e+aabb_min.y)/f;
      float t2 = (e+aabb_max.y)/f;

      if (t1>t2){float w=t1;t1=t2;t2=w;}

      if ( t2 < t_max )
        t_max = t2;
      if ( t1 > t_min )
        t_min = t1;
      if (t_min > t_max)
        return false;
    }
    else {
      if(-e+aabb_min.y > 0.0f || -e+aabb_max.y < 0.0f)
      return false;
    }
  }


  // Test intersection with the 2 planes perpendicular to the OBB's Z axis
  // Exactly the same thing than above.
  {
    glm::vec3 zaxis(model_matrix[2].x, model_matrix[2].y, model_matrix[2].z);
    float e = glm::dot(zaxis, delta);
    float f = glm::dot(ray_direction, zaxis);

    if ( fabs(f) > 0.001f ){

      float t1 = (e+aabb_min.z)/f;
      float t2 = (e+aabb_max.z)/f;

      if (t1>t2){float w=t1;t1=t2;t2=w;}

      if ( t2 < t_max )
      t_max = t2;
      if ( t1 > t_min )
      t_min = t1;
      if (t_min > t_max)
      return false;

    }
    else {
      if(-e+aabb_min.z > 0.0f || -e+aabb_max.z < 0.0f)
        return false;
    }
  }

  intersection_distance = t_min;
  return true;
}

} // namespace boomhs::collision
