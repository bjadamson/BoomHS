#pragma once
#include <boomhs/euler.hpp>
#include <boomhs/math_constants.hpp>

#include <extlibs/glm.hpp>

namespace boomhs::transform
{

template <typename T>
bool
is_rotated(T const& tr)
{
  return tr.rotation != glm::quat{};
}

template <typename T>
void
rotate_radians(T& transform, float const angle, glm::vec3 const& axis)
{
  auto& r = transform.rotation;
  r = glm::angleAxis(angle, axis) * r;
}

template <typename T>
void
rotate_radians(T& transform, float const angle, math::EulerAxis const axis)
{
  switch (axis) {
  case math::EulerAxis::X:
    rotate_radians(transform, angle, math::constants::X_UNIT_VECTOR);
    break;
  case math::EulerAxis::Y:
    rotate_radians(transform, angle, math::constants::Y_UNIT_VECTOR);
    break;
  case math::EulerAxis::Z:
    rotate_radians(transform, angle, math::constants::Z_UNIT_VECTOR);
    break;
  case math::EulerAxis::INVALID:
    std::abort();
    break;
  }
}

template <typename T>
void
rotate_xyz_radians(T& transform, glm::vec3 const& xyz_rot)
{
  rotate_radians(transform, xyz_rot.x, math::EulerAxis::X);
  rotate_radians(transform, xyz_rot.y, math::EulerAxis::Y);
  rotate_radians(transform, xyz_rot.z, math::EulerAxis::Z);
}

template <typename T>
void
rotate_xyz_radians(T& transform, float const x, float const y, float const z)
{
  rotate_xyz_radians(transform, glm::vec3{x, y, z});
}

template <typename T>
glm::vec3
get_rotation_radians(T const& transform)
{
  return glm::eulerAngles(transform.rotation);
}

template <typename T>
glm::vec3
get_rotation_degrees(T const& transform)
{
  return glm::degrees(get_rotation_radians(transform));
}

template <typename T>
void
rotate_degrees(T& transform, float const angle, glm::vec3 const& axis)
{
  rotate_radians(transform, glm::radians(angle), axis);
}

template <typename T>
void
rotate_degrees(T& transform, float const angle, math::EulerAxis const axis)
{
  rotate_radians(transform, glm::radians(angle), axis);
}

template <typename T>
void
rotate_xyz_degrees(T& transform, glm::vec3 const& xyz)
{
  rotate_xyz_radians(transform, glm::radians(xyz));
}

template <typename T>
void
rotate_xyz_degrees(T& transform, float const x, float const y, float const z)
{
  rotate_xyz_degrees(transform, glm::vec3{x, y, z});
}


} // namespace boomhs::transform

namespace boomhs
{

#define DECLARE_TRANSFORM_COMMON_MEMBER_FUNCTIONS                                                  \
  bool is_rotated() const { return transform::is_rotated(*this); }                                 \
  glm::vec3                                                                                        \
  get_rotation_degrees() const                                                                     \
  {                                                                                                \
    return transform::get_rotation_degrees(*this);                                                 \
  }                                                                                                \
                                                                                                   \
  glm::vec3                                                                                        \
  get_rotation_radians() const                                                                     \
  {                                                                                                \
    return transform::get_rotation_radians(*this);                                                 \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_degrees(float const angle, glm::vec3 const& axis)                                         \
  {                                                                                                \
    transform::rotate_degrees(*this, angle, axis);                                                 \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_degrees(float const angle, math::EulerAxis const axis)                                    \
  {                                                                                                \
    transform::rotate_degrees(*this, angle, axis);                                                 \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_radians(float const angle, glm::vec3 const& axis)                                         \
  {                                                                                                \
    transform::rotate_radians(*this, angle, axis);                                                 \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_radians(float const angle, math::EulerAxis const axis)                                    \
  {                                                                                                \
    transform::rotate_radians(*this, angle, axis);                                                 \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_xyz_degrees(glm::vec3 const& xyz)                                                         \
  {                                                                                                \
    transform::rotate_xyz_degrees(*this, xyz);                                                     \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_xyz_degrees(float const x, float const y, float const z)                                  \
  {                                                                                                \
    transform::rotate_xyz_degrees(*this, x, y, z);                                                 \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_xyz_radians(glm::vec3 const& xyz)                                                         \
  {                                                                                                \
    transform::rotate_xyz_radians(*this, xyz);                                                     \
  }                                                                                                \
                                                                                                   \
  void                                                                                             \
  rotate_xyz_radians(float const x, float const y, float const z)                                  \
  {                                                                                                \
    transform::rotate_xyz_radians(*this, x, y, z);                                                 \
  }

struct Transform2D
{
  // fields
  glm::vec2 translation;
  glm::quat rotation;
  glm::vec2 scale;

  Transform2D();
  Transform2D(float, float);
  Transform2D(glm::vec2 const&);

  // methods
  glm::mat4 model_matrix() const;
  DECLARE_TRANSFORM_COMMON_MEMBER_FUNCTIONS
};

struct Transform
{
  // fields
  glm::vec3 translation;
  glm::quat rotation;
  glm::vec3 scale;

  Transform();
  Transform(float, float, float);
  Transform(glm::vec3 const&);

  // methods
  void move(glm::vec3 const& delta)
  {
    translation += delta;
  }
  void move(float const x, float const y, float const z)
  {
    move(VEC3(x, y, z));
  }

  glm::mat4 model_matrix() const;
  DECLARE_TRANSFORM_COMMON_MEMBER_FUNCTIONS
};
#undef DECLARE_TRANSFORM_COMMON_MEMBER_FUNCTIONS

} // namespace boomhs

namespace boomhs::transform
{

// Utility/helper function to perform the common operation of lowering a Transform in 3dimensions
// to a 2d transform.
//
// This routine maps the x/y values to the output transform, discarding the z coordinates of the
// input transform.
inline Transform2D
from_3d_to_2d(Transform const& tr3d)
{
  Transform2D tr2d;
  tr2d.rotation = tr3d.rotation;
  {
    auto const& scale = tr3d.scale;
    tr2d.scale        = glm::vec2{scale.x, scale.y};
  }
  {
    auto const& pos_3d = tr3d.translation;
    auto& pos_2d       = tr2d.translation;

    pos_2d.x = pos_3d.x;
    pos_2d.y = pos_3d.y;
  }
  return tr2d;
}

} // namespace boomhs::transform
