#include <boomhs/math.hpp>
#include <boomhs/transform.hpp>

namespace
{

glm::vec3
three_axis_rotation(float const r11, float const r12, float const r21, float const r31, float const r32)
{
  glm::vec3 res;
  res[0] = std::atan2( r11, r12 );
  res[1] = std::acos ( r21 );
  res[2] = std::atan2( r31, r32 );
  return res;
}

glm::vec3
threeaxisrot(float const r11, float const r12, float const r21, float const r31, float const r32)
{
  glm::vec3 res;
  res[0] = std::atan2( r31, r32 );
  res[1] = std::asin ( r21 );
  res[2] = std::atan2( r11, r12 );
  return res;
}

} // namespace

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
Cube::scaled_dimensions(Transform const& tr) const
{
  auto const smin = scaled_min(tr);
  auto const smax = scaled_max(tr);
  return math::compute_cube_dimensions(smin, smax);
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
Cube::scaled_half_widths(Transform const& tr) const
{
  return scaled_dimensions(tr) / glm::vec3{2.0f};
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

std::string
Cube::to_string() const
{
  return fmt::sprintf("min: %s max %s", glm::to_string(min), glm::to_string(max));
}

std::ostream&
operator<<(std::ostream& ostream, Cube const& cube)
{
  ostream << cube.to_string();
  return ostream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Frustum
std::string
Frustum::to_string() const
{
  return fmt::sprintf("left: %f, right: %f, bottom: %f, top: %f, near: %f, far: %f",
      left, right, bottom, top, near, far);
}

Frustum
Frustum::from_rect_and_nearfar(RectInt const& rect, float const near, float const far)
{
  return Frustum{
    rect.float_left(),
    rect.float_right(),
    rect.float_bottom(),
    rect.float_top(),
    near,
    far};
}

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
  start = glm::normalize(start);
  dest  = glm::normalize(dest);

  float const cos_theta = glm::dot(start, dest);
  glm::vec3 axis;
  if (cos_theta < -1 + 0.001f) {
    // special case when vectors in opposite directions:
    // there is no "ideal" rotation axis
    // So guess one; any will do as long as it's perpendicular to start
    axis = glm::cross(constants::Z_UNIT_VECTOR, start);
    if (glm::length2(axis) < 0.01) {
      // bad luck, they were parallel, try again!
      axis = glm::cross(constants::X_UNIT_VECTOR, start);
    }

    axis = glm::normalize(axis);
    return glm::angleAxis(glm::radians(180.0f), axis);
  }
  axis = glm::cross(start, dest);

  float const s    = sqrt((1 + cos_theta) * 2);
  float const invs = 1 / s;

  return glm::quat{s * 0.5f, axis.x * invs, axis.y * invs, axis.z * invs};
}

glm::vec3
quat_to_euler(glm::quat const& q, RotSeq const rot_seq)
{
    switch(rot_seq) {
      case RotSeq::zyx:
      return threeaxisrot( 2*(q.x*q.y + q.w*q.z),
                     q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                    -2*(q.x*q.z - q.w*q.y),
                     2*(q.y*q.z + q.w*q.x),
                     q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z);
      break;

      case RotSeq::zyz:
      return three_axis_rotation( 2*(q.y*q.z - q.w*q.x),
                   2*(q.x*q.z + q.w*q.y),
                   q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                   2*(q.y*q.z + q.w*q.x),
                  -2*(q.x*q.z - q.w*q.y));
      break;

      case RotSeq::zxy:
      return threeaxisrot( -2*(q.x*q.y - q.w*q.z),
                      q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                      2*(q.y*q.z + q.w*q.x),
                     -2*(q.x*q.z - q.w*q.y),
                      q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z);
      break;

      case RotSeq::zxz:
      return three_axis_rotation( 2*(q.x*q.z + q.w*q.y),
                  -2*(q.y*q.z - q.w*q.x),
                   q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                   2*(q.x*q.z - q.w*q.y),
                   2*(q.y*q.z + q.w*q.x));
      break;

      case RotSeq::yxz:
      return threeaxisrot( 2*(q.x*q.z + q.w*q.y),
                     q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                    -2*(q.y*q.z - q.w*q.x),
                     2*(q.x*q.y + q.w*q.z),
                     q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z);
      break;

      case RotSeq::yxy:
      return three_axis_rotation( 2*(q.x*q.y - q.w*q.z),
                   2*(q.y*q.z + q.w*q.x),
                   q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                   2*(q.x*q.y + q.w*q.z),
                  -2*(q.y*q.z - q.w*q.x));
      break;

      case RotSeq::yzx:
      return threeaxisrot( -2*(q.x*q.z - q.w*q.y),
                      q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                      2*(q.x*q.y + q.w*q.z),
                     -2*(q.y*q.z - q.w*q.x),
                      q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z);
      break;

      case RotSeq::yzy:
      return three_axis_rotation( 2*(q.y*q.z + q.w*q.x),
                  -2*(q.x*q.y - q.w*q.z),
                   q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                   2*(q.y*q.z - q.w*q.x),
                   2*(q.x*q.y + q.w*q.z));
      break;

      case RotSeq::xyz:
      return threeaxisrot( -2*(q.y*q.z - q.w*q.x),
                    q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z,
                    2*(q.x*q.z + q.w*q.y),
                   -2*(q.x*q.y - q.w*q.z),
                    q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z);
      break;

      case RotSeq::xyx:
      return three_axis_rotation( 2*(q.x*q.y + q.w*q.z),
                  -2*(q.x*q.z - q.w*q.y),
                   q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                   2*(q.x*q.y - q.w*q.z),
                   2*(q.x*q.z + q.w*q.y));
      break;

      case RotSeq::xzy:
      return threeaxisrot( 2*(q.y*q.z + q.w*q.x),
                     q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z,
                    -2*(q.x*q.y - q.w*q.z),
                     2*(q.x*q.z + q.w*q.y),
                     q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z);
      break;

      case RotSeq::xzx:
      return three_axis_rotation( 2*(q.x*q.z - q.w*q.y),
                   2*(q.x*q.y + q.w*q.z),
                   q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z,
                   2*(q.x*q.z + q.w*q.y),
                  -2*(q.x*q.y - q.w*q.z));
      break;
    default:
      break;
   }
  std::abort();
  return constants::ZERO;
}


} // namespace boomhs::math
