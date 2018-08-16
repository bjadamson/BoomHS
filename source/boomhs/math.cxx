#include <boomhs/math.hpp>
#include <boomhs/transform.hpp>

#include <utility>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Cube
Cube::Cube(glm::vec3 const& minp, glm::vec3 const& maxp)
    : min(minp)
    , max(maxp)
{
}

glm::vec3
Cube::dimensions() const
{
  return math::compute_cube_dimensions(min, max);
}

glm::vec3
Cube::center() const
{
  auto const hw = half_widths();
  return glm::vec3{
    min.x + hw.x,
    min.y + hw.y,
    min.z + hw.z
  };
}

glm::vec3
Cube::half_widths() const
{
  return dimensions() / glm::vec3{2.0f};
}

glm::vec3
Cube::scaled_min(Transform const& tr) const
{
  auto const s  = tr.scale;
  auto const c  = center();
  auto const hw = half_widths();

  auto r = this->min;
  r.x = c.x - (s.x * hw.x);
  r.y = c.y - (s.y * hw.y);
  r.z = c.z - (s.z * hw.z);
  return r;
}

glm::vec3
Cube::scaled_max(Transform const& tr) const
{
  auto const s  = tr.scale;
  auto const c  = center();
  auto const hw = half_widths();

  auto r = this->max;
  r.x = c.x + (s.x * hw.x);
  r.y = c.y + (s.y * hw.y);
  r.z = c.z + (s.z * hw.z);
  return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Frustum

////////////////////////////////////////////////////////////////////////////////////////////////////
// Plane
float
Plane::dotproduct_with_vec3(Plane const& p, glm::vec3 const& v)
{
  return (p.a * v.x) + (p.b * v.y) + (p.c * v.z) + (p.d * 1.0);
}

float
Plane::dotproduct_with_vec3(glm::vec3 const& v) const
{
  return Plane::dotproduct_with_vec3(*this, v);
}

void
Plane::normalize()
{
  // Here we calculate the magnitude of the normal to the plane (point A B C)
  // Remember that (A, B, C) is that same thing as the normal's (X, Y, Z).
  // To calculate magnitude you use the equation:
  //     magnitude = sqrt(x^2 + y^2 + z^2)
  float const magnitude = std::sqrt(math::squared(a) + math::squared(b) + math::squared(c));

  // Then we normalize the plane by dividing the plane's values by it's magnitude.
  a /= magnitude;
  b /= magnitude;
  c /= magnitude;
  d /= magnitude;
}

#define MAP_INDEX_TO_PLANE_DATA_FIELD(I)                                                           \
  assert(i < 4);                                                                                   \
  if (i == 0) { return this->a; }                                                                  \
  if (i == 1) { return this->b; }                                                                  \
  if (i == 2) { return this->c; }                                                                  \
  if (i == 3) { return this->d; }                                                                  \
  std::abort()

float const&
Plane::operator[](size_t const i) const
{
  MAP_INDEX_TO_PLANE_DATA_FIELD(i);
}

float&
Plane::operator[](size_t const i)
{
  MAP_INDEX_TO_PLANE_DATA_FIELD(i);
}
#undef MAP_INDEX_TO_PLANE_DATA_FIELD

} // namespace boomhs

namespace boomhs::math
{

// Original source for algorithm.
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat
rotation_between_vectors(glm::vec3 start, glm::vec3 dest)
{
  using namespace glm;
  start = normalize(start);
  dest  = normalize(dest);

  float const cos_theta = dot(start, dest);
  vec3        axis;
  if (cos_theta < -1 + 0.001f) {
    // special case when vectors in opposite directions:
    // there is no "ideal" rotation axis
    // So guess one; any will do as long as it's perpendicular to start
    axis = cross(vec3(0.0f, 0.0f, 1.0f), start);
    if (glm::length2(axis) < 0.01) {
      // bad luck, they were parallel, try again!
      axis = cross(vec3(1.0f, 0.0f, 0.0f), start);
    }

    axis = normalize(axis);
    return angleAxis(radians(180.0f), axis);
  }
  axis = cross(start, dest);

  float const s    = sqrt((1 + cos_theta) * 2);
  float const invs = 1 / s;

  return glm::quat{s * 0.5f, axis.x * invs, axis.y * invs, axis.z * invs};
}

} // namespace boomhs::math
