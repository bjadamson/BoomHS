#pragma once
#include <boomhs/rectangle.hpp>
#include <common/algorithm.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glm.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <string>

namespace boomhs
{
struct Transform;

using ModelMatrix       = glm::mat4;
using ProjMatrix        = glm::mat4;
using ViewMatrix        = glm::mat4;
using ModelViewMatrix   = glm::mat4;

using TranslationMatrix = glm::mat4;
using ScaleMatrix       = glm::mat4;
using RotationMatrix    = glm::mat4;

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

namespace boomhs::math
{

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
compare_epsilon(float const a, float const b)
{
  return std::fabs(a - b) < constants::EPSILONF;
}

inline bool
is_unitv(glm::vec3 const& v)
{
  return compare_epsilon(glm::length(v), 1);
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

inline int
pythag_distance(glm::ivec2 const& a, glm::ivec2 const& b)
{
  // pythagorean theorem
  return std::sqrt(squared(b.x - a.x) + squared(b.y - a.y));
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
              RotationMatrix const& rot_matrix)
{
  auto const translate     = glm::translate(glm::mat4{}, rot_center);
  auto const inv_translate = glm::translate(glm::mat4{}, -rot_center);

  // The idea:
  // 1) Translate the object to the center
  // 2) Make the rotation
  // 3) Translate the object back to its original location
  auto const mm  = translate * rot_matrix * inv_translate;
  auto const pos = mm * glm::vec4{point_to_rotate, 1.0f};
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

inline ModelMatrix
compute_modelmatrix(glm::vec3 const& translation, glm::quat const& rotation, glm::vec3 const& scale)
{
  auto const tmatrix = glm::translate(glm::mat4{}, translation);
  auto const rmatrix = glm::toMat4(rotation);
  auto const smatrix = glm::scale(glm::mat4{}, scale);
  return tmatrix * rmatrix * smatrix;
}

inline ModelViewMatrix
compute_modelview_matrix(ModelMatrix const& model, ViewMatrix const& view)
{
  return view * model;
}

inline ModelViewMatrix
compute_mvp_matrix(ModelMatrix const& model, ViewMatrix const& view, ProjMatrix const& proj)
{
  return proj * view * model;
}

template <typename T>
inline constexpr void
assert_abs_lessthan_equal(T const& v, T const& max)
{
  assert(std::abs(v) <= max);
}

template <typename T>
inline constexpr void
assert_abs_lessthan_equal(glm::tvec3<T> const& v, T const& max)
{
  assert_abs_lessthan_equal(v.x, max);
  assert_abs_lessthan_equal(v.y, max);
  assert_abs_lessthan_equal(v.z, max);
}

} // namespace boomhs::math

namespace boomhs
{

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

  using CubeVertices = std::array<glm::vec3, 8>;
  CubeVertices scaled_cube_vertices(Transform const&) const;

  auto constexpr xz_rect() const { return RectFloat{min.x, min.z, max.x, max.z}; }
  auto constexpr xy_rect() const { return RectFloat{min.x, min.y, max.x, max.y}; }

  explicit Cube(glm::vec3 const&, glm::vec3 const&);

  std::string to_string() const;
};

std::ostream&
operator<<(std::ostream&, Cube const&);

struct Frustum
{
  int left, right, bottom, top;
  float near, far;

  float bottom_float() const { return bottom; }
  float top_float() const { return top; }

  float left_float() const { return left; }
  float right_float() const { return right; }

  auto height() const
  {
    assert(bottom > top);
    return std::abs(bottom - top);
  }
  auto width() const
  {
    assert(right > left);
    return std::abs(right - left);
  }
  auto constexpr depth() const
  {
    assert(far > near);
    return std::abs(far - near);
  }

  int width_int() const { return width(); }
  int height_int() const { return height(); }

  auto half_height() const { return height() / 2; }
  auto half_width() const { return width() / 2; }
  auto half_depth() const { return depth() / 2; }

  int half_height_int() const { return half_height(); }
  int half_width_int() const { return half_height(); }
  int half_depth_int() const { return half_height(); }

  auto dimensions() const { return glm::vec3(width(), height(), depth()); }
  auto center() const { return dimensions() / 2; }

  std::string to_string() const;

  static Frustum
  from_rect_and_nearfar(RectInt const&, float, float);
};

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

namespace boomhs::math
{

// Given two points, compute the "Absolute Value" rectangle.
//
// Essentially figures out which points are less-than the others and which points are greater than
// the others, and assigns then the values which will yield a rectangle.
//
// This function's purpose is to create a rectangle out of two points, with the top/left point
// always being <= to the bottom right point, even if the input points would naturally create a
// rectangle with the top/left being >= then the bottom/right
template <typename T>
auto
rect_abs_from_twopoints(glm::tvec2<T> const& a, glm::tvec2<T> const& b)
{
  auto const lx = math::lesser_of(a.x, b.x);
  auto const rx = common::other_of_two(lx, PAIR(a.x, b.x));

  auto const ty = math::lesser_of(a.y, b.y);
  auto const by = common::other_of_two(ty, PAIR(a.y, b.y));

  return RectFloat{VEC2{lx, ty}, VEC2{rx, by}};
}

} // namespace boomhs::math

namespace boomhs::math::space_conversions
{

auto constexpr DEFAULT_W = 1;

// from:
//   object space
// to:
//    world space
inline glm::vec4
object_to_world(glm::vec4 const& p, ModelMatrix const& model)
{
  return model * p;
}

inline glm::vec3
object_to_world(glm::vec3 const& p, ModelMatrix const& model)
{
  auto const v4 = glm::vec4{p, DEFAULT_W};

  return object_to_world(v4, model);
}

// from:
//   world space
// to:
//    eye space
inline glm::vec4
world_to_eye(glm::vec4 const& wp, ViewMatrix const& view)
{
  return view * wp;
}

inline glm::vec3
world_to_eye(glm::vec3 const& wp, ViewMatrix const& view)
{
  auto const v4 = glm::vec4{wp, DEFAULT_W};
  return world_to_eye(v4, view);
}

// from:
//   eye space
// to:
//    clip space
inline glm::vec4
eye_to_clip(glm::vec4 const& eyep, ProjMatrix const& proj)
{
  return proj * eyep;
}

inline glm::vec3
eye_to_clip(glm::vec3 const& eyep, ProjMatrix const& proj)
{
  auto const v4 = glm::vec4{eyep, DEFAULT_W};
  return eye_to_clip(v4, proj);
}

// from:
//   clip space
// to:
//    ndc space
inline glm::vec3
clip_to_ndc(glm::vec4 const& clip)
{
  auto const ndc4 = clip / clip.w;
  return glm::vec3{ndc4.x, ndc4.y, ndc4.z};
}

inline glm::vec3
clip_to_ndc(glm::vec3 const& clip)
{
  glm::vec4 const clip4{clip, DEFAULT_W};

  return clip_to_ndc(clip4);
}

// Convert a point in screen/window space to the viewport's space.
//
// "ViewportSpace" defined as an offset from the screen's origin (0, 0) being the top-left, a
// simple subtraction is required.
inline glm::ivec2
screen_to_viewport(glm::ivec2 const& point, glm::ivec2 const& origin)
{
  return point - origin;
}

inline glm::ivec2
screen_to_viewport(glm::vec2 const& point, glm::ivec2 const& origin)
{
  glm::ivec2 const point_i{point};
  return screen_to_viewport(point_i, origin);
}

inline glm::vec4
clip_to_eye(glm::vec4 const& clip, ProjMatrix const& pm, float const z)
{
  auto const      inv_proj   = glm::inverse(pm);
  glm::vec4 const eye_coords = inv_proj * clip;
  return glm::vec4{eye_coords.x, eye_coords.y, z, 0.0f};
}

inline glm::vec3
eye_to_world(glm::vec4 const& eye, ViewMatrix const& vm)
{
  auto const inv_view       = glm::inverse(vm);
  glm::vec4 const ray       = inv_view * eye;
  glm::vec3 const ray_world = glm::vec3{ray.x, ray.y, ray.z};
  return glm::normalize(ray_world);
}

// from:
//    ndc
// to:
//    screen/window
inline constexpr glm::vec2
screen_to_ndc(glm::vec2 const& scoords, RectFloat const& viewport)
{
  float const x = ((2.0f * scoords.x) / viewport.right) - 1.0f;
  float const y = ((2.0f * scoords.y) / viewport.bottom) - 1.0f;

  assert_abs_lessthan_equal(x, 1.0f);
  assert_abs_lessthan_equal(y, 1.0f);
  return glm::vec2{x, -y};
}

//
// Algorithm modified from:
// http://www.songho.ca/opengl/gl_transform.html
inline glm::vec2
ndc_to_screen(glm::vec3 const& ndc, RectFloat const& viewport)
{
  auto const origin    = viewport.left_top();
  auto const view_rect = viewport.size();
  return glm::vec2{
    (((ndc.x + 1 ) / 2) * view_rect.x) + origin.x,
    ((((1 - ndc.y) / 2) * view_rect.y) + origin.y)
  };
}

// from:
//    ndc
// to:
//    clip
inline glm::vec4
ndc_to_clip(glm::vec2 const& ndc, float const z)
{
  return glm::vec4{ndc.x, ndc.y, z, 1.0f};
}

// from:
//    screen/window
// to:
//    world
inline glm::vec3
screen_to_world(glm::vec2 const& scoords, RectFloat const& viewport, ProjMatrix const& pm,
                ViewMatrix const& vm, float const z)
{
  glm::vec2 const ndc   = screen_to_ndc(scoords, viewport);
  glm::vec4 const clip  = ndc_to_clip(ndc, z);
  glm::vec4 const eye   = clip_to_eye(clip, pm, z);
  glm::vec3 const world = eye_to_world(eye, vm);
  return world;
}

// from:
//   object space
// to:
//    eye space
inline glm::vec4
object_to_eye(glm::vec4 const& p, ViewMatrix const& view, ModelMatrix const& model)
{
  auto const wp = object_to_world(p, model);
  return world_to_eye(wp, view);
}

inline glm::vec4
object_to_eye(glm::vec3 const& p, ViewMatrix const& view, ModelMatrix const& model)
{
  auto const v4 = glm::vec4{p, DEFAULT_W};
  return object_to_eye(v4, view, model);
}

// from:
//   object space
// to:
//    ndc space
inline glm::vec3
object_to_ndc(glm::vec4 const& p, ModelMatrix const& model, ProjMatrix const& proj,
              ViewMatrix const& view)
{
  auto const eye = object_to_eye(p, view, model);
  auto const clip = eye_to_clip(eye, proj);

  return clip_to_ndc(clip);
}

inline glm::vec3
object_to_ndc(glm::vec3 const& p, ModelMatrix const& model, ProjMatrix const& proj,
              ViewMatrix const& view)
{
  glm::vec4 const p4{p, DEFAULT_W};

  return object_to_ndc(p4, model, proj, view);
}

// from:
//   object space
// to:
//    screen/window space
inline glm::vec2
object_to_screen(glm::vec4 const& p, ModelMatrix const& model, ProjMatrix const& proj,
                 ViewMatrix const& view, RectFloat const& viewport)
{
  // TODO: This function is a hack it's implemementation first steps not being what I expect I
  // should try and understand what's going on ASAP.
  auto const mvp  = proj * view;
  auto const clip = mvp * p;
  auto const ndc  = clip_to_ndc(clip);
  return ndc_to_screen(ndc, viewport);
}

inline glm::vec2
object_to_screen(glm::vec3 const& p, ModelMatrix const& model, ProjMatrix const& proj,
                 ViewMatrix const& view, RectFloat const& viewport)
{
  glm::vec4 const p4{p, DEFAULT_W};

  return object_to_screen(p4, model, proj, view, viewport);
}

// from:
//   object space
// to:
//    viewport space
inline glm::vec2
object_to_viewport(glm::vec4 const& p, ModelMatrix const& model, ProjMatrix const& proj,
                 ViewMatrix const& view, RectFloat const& viewport)
{
  auto const ss = object_to_screen(p, model, proj, view, viewport);
  return screen_to_viewport(ss, viewport.left_top());
}

inline glm::vec2
object_to_viewport(glm::vec3 const& p, ModelMatrix const& model, ProjMatrix const& proj,
                 ViewMatrix const& view, RectFloat const& viewport)
{
  glm::vec4 const p4{p, DEFAULT_W};
  return object_to_viewport(p4, model, proj, view, viewport);
}

} // namespace boomhs::math::space_conversions
