#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/math.hpp>
#include <boomhs/polygon.hpp>
#include <boomhs/transform.hpp>
#include <boomhs/viewport.hpp>

#include <common/algorithm.hpp>
#include <limits>

using namespace boomhs;
using namespace boomhs::math;

static auto constexpr FLOAT_MIN = std::numeric_limits<float>::max();
static auto constexpr FLOAT_MAX = std::numeric_limits<float>::min();

namespace
{

// algorithm adapted from:
// http://www.opengl-tutorial.org/kr/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
// https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_custom.cpp
bool
ray_obb_intersection(
    Ray const&       ray,
    glm::vec3 const& aabb_min,   // Minimum X,Y,Z coords of the mesh when not transformed at all.
    glm::vec3 const& aabb_max,   // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is
                                 // centered, but it's not always the case.
    ModelMatrix const& mm,       // Transformation applied to the mesh (which will thus be also
    float& intersection_distance // Output : distance between ray_origin and the OBB intersection.
)
{
  // Intersection method from Real-Time Rendering and Essential Mathematics for Games
  float t_min = 0.0f;
  float t_max = 100000.0f;

  glm::vec3 const OBB_position_worldspace(mm[3].x, mm[3].y, mm[3].z);
  glm::vec3 const delta = OBB_position_worldspace - ray.origin;

#define TEST_PLANE_INTERSECTION_IMPL(INDEX, AABB_MIN, AABB_MAX)                                    \
  {                                                                                                \
    glm::vec3 const axis(mm[INDEX].x, mm[INDEX].y, mm[INDEX].z);                                   \
    float const     e = glm::dot(axis, delta);                                                     \
    float const     f = glm::dot(ray.direction, axis);                                             \
                                                                                                   \
    if (std::fabs(f) > 0.001f) {     /* Standard case */                                           \
      float t1 = (e + AABB_MIN) / f; /* Intersection with the "left" plane */                      \
      float t2 = (e + AABB_MAX) / f; /* Intersection with the "right" plane */                     \
      /* t1 and t2 now contain distances betwen ray origin and ray-plane intersections */          \
                                                                                                   \
      /* We want t1 to represent the nearest intersection, */                                      \
      /* so if it's not the case, invert t1 and t2*/                                               \
      if (t1 > t2) {                                                                               \
        float const temp = t1;                                                                     \
        t1               = t2;                                                                     \
        t2               = temp; /* swap t1 and t2*/                                               \
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
    else if ((-e + AABB_MIN) > 0.0f || (-e + AABB_MAX) < 0.0f) {                                   \
      /* Rare case : the ray is almost parallel to the planes, so they don't have any */           \
      /* intersection. */                                                                          \
      return false;                                                                                \
    }                                                                                              \
  }

  // Test intersection with the 2 planes perpendicular to the OBB's 3 axis
  TEST_PLANE_INTERSECTION_IMPL(0, aabb_min.x, aabb_max.x);
  TEST_PLANE_INTERSECTION_IMPL(1, aabb_min.y, aabb_max.y);
  TEST_PLANE_INTERSECTION_IMPL(2, aabb_min.z, aabb_max.z);
#undef TEST_PLANE_INTERSECTION_IMPL

  intersection_distance = t_min;
  return true;
}

bool
ray_obb_intersection(Ray const& ray, Cube cube, Transform tr, float& distance)
{
  auto const c  = cube.center();
  auto const hw = cube.half_widths();
  auto const s  = tr.scale;

  // calculate where the min/max values are from the center of the object after scaling.
  auto const min = cube.scaled_min(tr);
  auto const max = cube.scaled_max(tr);

  // For the purposes of the ray_obb intersection algorithm, it is expected the transform has no
  // scaling. We've taking the scaling into account by adjusting the bounding cube min/max points
  // using the transform's original scale. Set the scaling of the copied transform to all 1's.
  tr.scale                = glm::vec3{1};
  auto const model_matrix = tr.model_matrix();
  return ray_obb_intersection(ray, min, max, model_matrix, distance);
}

// Determine whether a ray and an Axis Aligned Cube
// algorithm adopted from:
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-cube-intersection
bool
ray_axis_aligned_cube_intersect(Ray const& r, Transform const& transform, Cube const& cube,
                                float& distance)
{
  auto const& cubepos = transform.translation;

  glm::vec3 const                minpos = cube.min * transform.scale;
  glm::vec3 const                maxpos = cube.max * transform.scale;
  std::array<glm::vec3, 2> const bounds{{minpos + cubepos, maxpos + cubepos}};

  // clang-format off
  float txmin = (bounds[    r.sign[0]].x - r.origin.x) * r.invdir.x;
  float txmax = (bounds[1 - r.sign[0]].x - r.origin.x) * r.invdir.x;
  float tymin = (bounds[    r.sign[1]].y - r.origin.y) * r.invdir.y;
  float tymax = (bounds[1 - r.sign[1]].y - r.origin.y) * r.invdir.y;

  if ((txmin > tymax) || (tymin > txmax)) {
    return false;
  }
  if (tymin > txmin) {
    txmin = tymin;
  }
  if (tymax < txmax) {
    txmax = tymax;
  }

  float tzmin = (bounds[    r.sign[2]].z - r.origin.z) * r.invdir.z;
  float tzmax = (bounds[1 - r.sign[2]].z - r.origin.z) * r.invdir.z;
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

template <typename V>
auto
rotated_rectangle_points(RectT<V> const& rect, ModelMatrix const& model)
{
  auto const& [p0, p1, p2, p3] = rect.points().vertices;
  auto constexpr Z             = 0;
  auto const make_3d           = [](auto const& p) { return glm::vec3{p.x, p.y, Z}; };

  // Transform the points into eye space
  namespace sv = space_conversions;
  auto const v0 = sv::object_to_world(make_3d(p0), model);
  auto const v1 = sv::object_to_world(make_3d(p1), model);
  auto const v2 = sv::object_to_world(make_3d(p2), model);
  auto const v3 = sv::object_to_world(make_3d(p3), model);

  // Discard the extra vertices, rectangles only have two dimensions.
  using VerticesT = typename RectT<V>::vertex_type;
  auto const make_2d = [](auto const& p) { return VerticesT{p.x, p.y}; };
  auto const v0_2d = make_2d(v0);
  auto const v1_2d = make_2d(v1);
  auto const v2_2d = make_2d(v2);
  auto const v3_2d = make_2d(v3);

  // Combine the rectangles vertices into an array and return.
  using VerticesArray = typename RectT<V>::array_type;
  return VerticesArray{v0_2d, v1_2d, v2_2d, v3_2d};
}

// Determine whether two (possibly rotated) polygons overlap eachother.
// Algorithm mutated from:
// https://www.codeproject.com/Articles/15573/2D-Polygon-Collision-Detection
template <typename V, size_t N>
bool
overlap_polygon(PolygonVertices<V, N> const& a, PolygonVertices<V, N> const& b)
{
  auto const calc_minmax = [](auto const& polygon, auto const& normal) {
    float min = FLOAT_MIN, max = FLOAT_MAX;
    for (auto const& p : polygon) {
      auto const projected = (normal.x * p.x) + (normal.y * p.y);
      if (projected < min) {
        min = projected;
      }
      if (projected > max) {
        max = projected;
      }
    }
    return PAIR(min, max);
  };

  auto const projected_axis_overlap = [&](auto const& polygon, auto const i0) {
    auto const polygon_vertex_count = polygon.size();
    auto const i1 = (i0 + 1) % polygon_vertex_count;
    auto const& p1 = polygon[i0];
    auto const& p2 = polygon[i1];

    glm::vec2 const normal{p2.y - p1.y, p1.x - p2.x};

    auto const [min_a, max_a] = calc_minmax(a, normal);
    auto const [min_b, max_b] = calc_minmax(b, normal);

    return !(max_a < min_b || max_b < min_a);
  };

  std::array<PolygonVertices<V, N>, 2> const rects{{a, b}};
  for (auto const& polygon : rects) {
    auto const polygon_vertex_count = polygon.size();
    FOR(i, polygon_vertex_count)
    {
      if (!projected_axis_overlap(polygon, i)) {
        return false;
      }
    }
  }
  // overlapping
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// 3D OBB Collision helper code.
using SeperatedVertices = std::array<glm::vec3, 8>;
bool
is_separated(SeperatedVertices const& vertsA, SeperatedVertices const& vertsB, glm::vec3 const& axis)
{
  // Handles the cross product = {0,0,0} case
  if (axis == constants::ZERO) {
    return false;
  }

  auto aMin = FLOAT_MIN;
  auto aMax = FLOAT_MAX;
  auto bMin = FLOAT_MIN;
  auto bMax = FLOAT_MAX;

  // Define two intervals, a and b. Calculate their min and max values
  assert(8 == vertsA.size());
  assert(8 == vertsB.size());

  FOR(i, 8)
  {
    auto const aDist = glm::dot(vertsA[i], axis);

    aMin = aDist < aMin ? aDist : aMin;
    aMax = aDist > aMax ? aDist : aMax;

    auto const bDist = glm::dot(vertsB[i], axis);

    bMin = bDist < bMin ? bDist : bMin;
    bMax = bDist > bMax ? bDist : bMax;
  }

  // One-dimensional intersection test between a and b
  auto const longSpan = glm::max(aMax, bMax) - glm::min(aMin, bMin);
  auto const sumSpan = aMax - aMin + bMax - bMin;
  return longSpan >= sumSpan; // > to treat touching as intersection
}

auto
compute_obb_vertices(glm::vec3 const& center, glm::vec3 const& size, glm::quat const& rot)
{
  auto const max = size / 2;
  auto const min = -max;

  return common::make_array<glm::vec3>(
    center + rot * min,
    center + rot * glm::vec3{max.x, min.y, min.z},
    center + rot * glm::vec3{min.x, max.y, min.z},
    center + rot * glm::vec3{max.x, max.y, min.z},
    center + rot * glm::vec3{min.x, min.y, max.z},
    center + rot * glm::vec3{max.x, min.y, max.z},
    center + rot * glm::vec3{min.x, max.y, max.z},
    center + rot * max
    );
}

} // namespace

namespace boomhs
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// Ray
Ray::Ray(glm::vec3 const& o, glm::vec3 const& d)
    : origin(o)
    , direction(d)
    , invdir(1.0f / direction)
    , sign(common::make_array<int>(invdir.x < 0, invdir.y < 0, invdir.z < 0))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// OBB
OBB::OBB(glm::vec3 const& center, glm::vec3 const& size, glm::quat const& rot)
    : right(rot * constants::X_UNIT_VECTOR)
    , up(rot * constants::Y_UNIT_VECTOR)
    , forward(rot * constants::Z_UNIT_VECTOR)
    , vertices(compute_obb_vertices(center, size, rot))
{
}

OBB
OBB::from_cube_transform(Cube const& cube, Transform const& tr)
{
  auto const cpos = tr.translation + cube.center();
  auto const& rot = tr.rotation;
  auto const ssize = cube.scaled_dimensions(tr);
  return OBB{cpos, ssize, rot};
}

} // namespace boomhs

namespace boomhs::collision
{

bool
intersects(glm::vec2 const& point, RectFloat const& rect)
{
  bool const within_lr = point.x >= rect.left && point.x <= rect.right;
  bool const within_tb = point.y >= rect.top && point.y <= rect.bottom;

  return within_lr && within_tb;
}

bool
intersects(common::Logger& logger, Ray const& ray, Transform const& tr, Cube const& cube,
           float& distance)
{
  bool const can_use_simple_test = (tr.rotation == glm::quat{}) && (tr.scale == constants::ONE);

  bool       intersects       = false;
  auto const log_intersection = [&](char const* test_name) {
    if (intersects) {
      LOG_ERROR_SPRINTF("Intersection found using %s test, distance %f", test_name, distance);
    }
  };

  if (can_use_simple_test) {
    intersects = ray_axis_aligned_cube_intersect(ray, tr, cube, distance);
    //log_intersection("SIMPLE");
  }
  else {
    intersects = ray_obb_intersection(ray, cube, tr, distance);
    //log_intersection("COMPLEX");
  }
  return intersects;
}

bool
overlap_axis_aligned(RectFloat const& a, RectFloat const& b)
{
  return a.left < b.right && a.right > b.left && a.bottom > b.top && a.top < b.bottom;
}

bool
overlap_axis_aligned(common::Logger& logger, CubeTransform const& a, CubeTransform const& b)
{
  auto const& at = a.transform;
  auto const& bt = b.transform;

  auto const ah = a.cube.half_widths() * at.scale;
  auto const bh = b.cube.half_widths() * bt.scale;

  auto const& att = at.translation;
  auto const& btt = bt.translation;
  bool const  x   = std::abs(att.x - btt.x) <= (ah.x + bh.x);
  bool const  y   = std::abs(att.y - btt.y) <= (ah.y + bh.y);
  bool const  z   = std::abs(att.z - btt.z) <= (ah.z + bh.z);

  return x && y && z;
}

// Compare the rectangle 'a' with the rectangle within 'rb' after applying the transform to the 
bool
overlap(RectFloat const& a, RectTransform const& rb, Viewport const& viewport, bool const is_2d)
{
  Transform2D ta;
  RectTransform const ra{a, ta};

  auto const& rbr = rb.rect;
  auto const& rbt = rb.transform.translation;

  bool const is_axis_aligned = !rb.transform.is_rotated();
  if (is_axis_aligned) {
    auto lt           = rbr.left_top_scaled(rb.transform);
    auto br           = rbr.right_bottom_scaled(rb.transform);
    auto const origin = viewport.left_top();

    if (is_2d) {
      lt -= origin;
      br -= origin;
    }

    RectFloat const new_rectb{
      lt.x  + rbt.x,
      lt.y  + rbt.y,

      br.x + rbt.x,
      br.y + rbt.y
    };

    Transform2D tb;
    tb.rotation = rb.transform.rotation;
    RectTransform const rb_new{new_rectb, tb};

    return overlap(ra, rb_new);
  }

  auto rb_rect = rb.rect;
  Transform2D ZZZ;
  if (is_2d) {
    auto const origin = viewport.left_top();
    ZZZ.translation.x = origin.x;
    ZZZ.translation.y = origin.y;
  }

  Transform2D tb = rb.transform;
  tb.rotation    = rb.transform.rotation;
  RectTransform const rb_new{rb_rect, tb};

  RectTransform const ra_rt{a, ZZZ};
  return overlap(ra_rt, rb_new);
}

bool
overlap(RectTransform const& a, RectTransform const& b)
{
  auto const& ra = a.rect;
  auto const& rb = b.rect;

  auto const& ta = a.transform;
  auto const& tb = b.transform;

  // Only perform the simple test if both rectangles ARE NOT rotated.
  bool const simple_test = !ta.is_rotated() && !tb.is_rotated();

  // Rotate the rectangles using their vertices/transform and proj matrix.
  auto const a_points = rotated_rectangle_points(ra, ta.model_matrix());
  auto const b_points = rotated_rectangle_points(rb, tb.model_matrix());

  // There are 4 vertices in a rectangle.
  constexpr auto N = 4;
  return simple_test ? overlap_axis_aligned(ra, rb)
                     : overlap_polygon<glm::vec2, N>(a_points, b_points);
}

// Determine if two OBB's overlap.
//
// algorithm modified from source:
// https://gamedev.stackexchange.com/a/165158/24796
bool
overlap(OBB const& a, OBB const& b)
{

// Define helper macro for hiding this repeated logic
// RF_IF => "return false if"
#define RF_IF_SEPARATED(...) if (is_separated(__VA_ARGS__)) { return false; }
  RF_IF_SEPARATED(a.vertices, b.vertices, a.right);

  RF_IF_SEPARATED(a.vertices, b.vertices, a.right);
  RF_IF_SEPARATED(a.vertices, b.vertices, a.up);
  RF_IF_SEPARATED(a.vertices, b.vertices, a.forward);

  RF_IF_SEPARATED(a.vertices, b.vertices, b.right);
  RF_IF_SEPARATED(a.vertices, b.vertices, b.up);
  RF_IF_SEPARATED(a.vertices, b.vertices, b.forward);

  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.right, b.right));
  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.right, b.up));
  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.right, b.forward));

  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.up, b.right));
  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.up, b.up));
  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.up, b.forward));

  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.forward, b.right));
  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.forward, b.up));
  RF_IF_SEPARATED(a.vertices, b.vertices, glm::cross(a.forward, b.forward));
#undef RF_IF_SEPARATED

  // None of the axis are seperated, the two OBB's must overlap.
  return true;
}

} // namespace boomhs::collision
