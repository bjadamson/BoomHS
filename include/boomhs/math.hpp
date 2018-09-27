#pragma once
#include <extlibs/glm.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <ostream>
#include <string>

namespace boomhs
{
struct Transform;

struct Rectangle
{
  // fields
  glm::vec2 min, max;

  // ctor
  explicit constexpr Rectangle(int const l, int const t, int const r, int const b)
      : Rectangle(static_cast<float>(l), static_cast<float>(t), static_cast<float>(r),
                  static_cast<float>(b))
  {
  }

  explicit constexpr Rectangle(float const l, float const t, float const r, float const b)
      : Rectangle(glm::vec2{l, t}, glm::vec2{r, b})
  {
  }

  explicit constexpr Rectangle(glm::vec2 const& p0, glm::vec2 const& p1)
      : min(p0)
      , max(p1)
  {
  }

  // methods
  auto constexpr left() const { return min.x; }
  auto constexpr top() const { return min.y; }

  auto constexpr right() const { return max.x; }
  auto constexpr bottom() const { return max.y; }

  auto constexpr left_top() const { return glm::vec2{left(), top()}; }
  auto constexpr left_bottom() const { return glm::vec2{left(), bottom()}; }

  auto constexpr right_top() const { return glm::vec2{right(), top()}; }
  auto constexpr right_bottom() const { return glm::vec2{right(), bottom()}; }

  auto width() const { return std::abs(right() - left()); }
  auto height() const { return std::abs(top() - bottom()); }

  std::string to_string() const;
};

struct Cube
{
  glm::vec3 min, max;

  // methods
  glm::vec3 dimensions() const;
  glm::vec3 center() const;
  glm::vec3 half_widths() const;

  glm::vec3 scaled_min(Transform const&) const;
  glm::vec3 scaled_max(Transform const&) const;
  glm::vec3 scaled_dimensions(Transform const&) const;
  glm::vec3 scaled_half_widths(Transform const&) const;

  Rectangle constexpr xz_rect() const { return Rectangle{min.x, min.z, max.x, max.z}; }

  explicit Cube(glm::vec3 const&, glm::vec3 const&);

  std::string to_string() const;
};

std::ostream&
operator<<(std::ostream&, Cube const&);

struct Frustum
{
  float left, right, bottom, top, near, far;

  auto height() const
  {
    assert(bottom > top);
    return bottom - top;
  }
  auto width() const
  {
    assert(right > left);
    return right - left;
  }
  auto depth() const
  {
    assert(far > near);
    return far - near;
  }

  auto dimensions() const { return glm::vec3(width(), height(), depth()); }

  glm::vec3 center() const { return dimensions() / 2.0f; }

  std::string to_string() const;
};

Frustum
operator/(Frustum const&, Frustum const&);

// We create an enum of the sides so we don't have to call each side 0 or 1.
// This way it makes it more understandable and readable when dealing with frustum sides.
enum FrustumSide
{
  RIGHT  = 0, // The RIGHT  side of the frustum
  LEFT   = 1, // The LEFT   side of the frustum
  BOTTOM = 2, // The BOTTOM side of the frustum
  TOP    = 3, // The TOP    side of the frustum
  BACK   = 4, // The BACK   side of the frustum
  FRONT  = 5  // The FRONT  side of the frustum
};

struct Plane
{
  // a => The X value of the plane's normal
  // b => The Y value of the plane's normal
  // c => The Z value of the plane's normal
  // d => The distance the plane is from the origin
  float a, b, c, d;

  float const& operator[](size_t) const;
  float&       operator[](size_t);

  // Compute dot product of the plane and 3D vector.
  // Given a plane (a, b, c, d) and a 3D vector (x, y, z) the return value of this function is:
  //     a*x + b*y + c*z + d*1.
  // The dotproduct_with_vec3 function is useful for determining the plane's relationship with a
  // coordinate in 3D space.
  //
  // source:
  // https://docs.microsoft.com/en-us/windows/desktop/direct3d9/d3dxplanedotcoord
  float        dotproduct_with_vec3(glm::vec3 const&) const;
  static float dotproduct_with_vec3(Plane const&, glm::vec3 const&);

  void normalize();
};

} // namespace boomhs

namespace boomhs::math::constants
{
auto constexpr ZERO = glm::vec3{0};
auto constexpr ONE  = glm::vec3{1};

auto constexpr X_UNIT_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
auto constexpr Y_UNIT_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
auto constexpr Z_UNIT_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};

auto constexpr PI       = glm::pi<float>();
auto constexpr TWO_PI   = PI * 2.0f;
auto constexpr EPSILONF = std::numeric_limits<float>::epsilon();

} // namespace boomhs::math::constants

namespace boomhs::math::space_conversions
{

inline glm::vec4
clip_to_eye(glm::vec4 const& clip, glm::mat4 const& proj_matrix, float const z)
{
  auto const      inv_proj   = glm::inverse(proj_matrix);
  glm::vec4 const eye_coords = inv_proj * clip;
  return glm::vec4{eye_coords.x, eye_coords.y, z, 0.0f};
}

inline glm::vec3
eye_to_world(glm::vec4 const& eye, glm::mat4 const& view_matrix)
{
  glm::mat4 const inv_view  = glm::inverse(view_matrix);
  glm::vec4 const ray       = inv_view * eye;
  glm::vec3 const ray_world = glm::vec3{ray.x, ray.y, ray.z};
  return glm::normalize(ray_world);
}

inline constexpr glm::vec2
screen_to_ndc(glm::vec2 const& scoords, Rectangle const& view_rect)
{
  float const x = ((2.0f * scoords.x) / view_rect.right()) - 1.0f;
  float const y = ((2.0f * scoords.y) / view_rect.bottom()) - 1.0f;

  auto const assert_fn = [](float const v) {
    assert(v <= 1.0f);
    assert(v >= -1.0f);
  };
  assert_fn(x);
  assert_fn(y);
  return glm::vec2{x, -y};
}

inline glm::vec4
ndc_to_clip(glm::vec2 const& ndc, float const z)
{
  return glm::vec4{ndc.x, ndc.y, z, 1.0f};
}

inline glm::vec3
screen_to_world(glm::vec2 const& scoords, Rectangle const& view_rect, glm::mat4 const& proj_matrix,
                glm::mat4 const& view_matrix, float const z)
{
  glm::vec2 const ndc   = screen_to_ndc(scoords, view_rect);
  glm::vec4 const clip  = ndc_to_clip(ndc, z);
  glm::vec4 const eye   = clip_to_eye(clip, proj_matrix, z);
  glm::vec3 const world = eye_to_world(eye, view_matrix);
  return world;
}

} // namespace boomhs::math::space_conversions

namespace boomhs::math
{

enum class EulerAxis
{
  X = 0,
  Y,
  Z,
  INVALID
};

template <typename ...T>
inline void
negate(T&&... values)
{
  auto const negate_impl = [](auto &v) { v = -v; };
  (negate_impl(values), ...);
}

template <typename T>
inline auto
squared(T const& value)
{
  return value * value;
}

inline bool
opposite_signs(int const x, int const y)
{
  return ((x ^ y) < 0);
}

template <typename T>
T const&
lesser_of(T const& a, T const& b)
{
  return a < b ? a : b;
}

inline bool
float_compare(float const a, float const b)
{
  return std::fabs(a - b) < std::numeric_limits<float>::epsilon();
}

inline glm::vec3
lerp(glm::vec3 const& a, glm::vec3 const& b, float const f)
{
  return (a * (1.0 - f)) + (b * f);
}

inline bool
allnan(glm::vec3 const& v)
{
  return std::isnan(v.x) && std::isnan(v.y) && std::isnan(v.z);
}

inline bool
anynan(glm::vec3 const& v)
{
  return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
}

// Normalizes "value" from the "from_range" to the "to_range"
template <typename T, typename P1, typename P2>
constexpr float
normalize(T const value, P1 const& from_range, P2 const& to_range)
{
  static_assert(std::is_integral<T>::value, "Input must be integral");
  auto const minimum    = from_range.first;
  auto const maximum    = from_range.second;
  auto const floor      = to_range.first;
  auto const ceil       = to_range.second;
  auto const normalized = ((ceil - floor) * (value - minimum)) / (maximum - minimum) + floor;
  return static_cast<float>(normalized);
}

inline float
angle_between_vectors(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& origin)
{
  glm::vec3 const da = glm::normalize(a - origin);
  glm::vec3 const db = glm::normalize(b - origin);

  return std::acos(glm::dot(da, db));
}

inline glm::vec3
rotate_around(glm::vec3 const& point_to_rotate, glm::vec3 const& rot_center,
              glm::mat4x4 const& rot_matrix)
{
  glm::mat4x4 const translate     = glm::translate(glm::mat4{}, rot_center);
  glm::mat4x4 const inv_translate = glm::translate(glm::mat4{}, -rot_center);

  // The idea:
  // 1) Translate the object to the center
  // 2) Make the rotation
  // 3) Translate the object back to its original location
  glm::mat4x4 const transform = translate * rot_matrix * inv_translate;
  auto const        pos       = transform * glm::vec4{point_to_rotate, 1.0f};
  return glm::vec3{pos.x, pos.y, pos.z};
}

// Original source for algorithm.
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat
rotation_between_vectors(glm::vec3 start, glm::vec3 dest);

// Calculates the cube's dimensions, given a min/max point in 3D.
//
// Interpret the glm::vec3 returned like:
//   x == width
//   y == height
//   z == length
inline glm::vec3
compute_cube_dimensions(glm::vec3 const& min, glm::vec3 const& max)
{
  auto const w = std::abs(max.x - min.x);
  auto const h = std::abs(max.y - min.y);
  auto const l = std::abs(max.z - min.z);
  return glm::vec3{w, h, l};
}

inline glm::mat4
compute_modelmatrix(glm::vec3 const& translation, glm::quat const& rotation, glm::vec3 const& scale)
{
  auto const tmatrix = glm::translate(glm::mat4{}, translation);
  auto const rmatrix = glm::toMat4(rotation);
  auto const smatrix = glm::scale(glm::mat4{}, scale);
  return tmatrix * rmatrix * smatrix;
}

inline glm::mat4
compute_modelview_matrix(glm::mat4 const& model, glm::mat4 const& view)
{
  return view * model;
}

inline glm::mat4
compute_mvp_matrix(glm::mat4 const& model, glm::mat4 const& view, glm::mat4 const& proj)
{
  return proj * view * model;
}

///////////////////////////////
// Quaternion to Euler
//
// algorithm modified from source:
// http://bediyap.com/programming/convert-quaternion-to-euler-rotations/
///////////////////////////////
enum class RotSeq
{
  zyx,
  zyz,
  zxy,
  zxz,
  yxz,
  yxy,
  yzx,
  yzy,
  xyz,
  xyx,
  xzy,
  xzx
};
glm::vec3
quat_to_euler(glm::quat const&, RotSeq);

} // namespace boomhs::math
