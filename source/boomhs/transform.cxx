#include <boomhs/transform.hpp>

using namespace boomhs;
using namespace boomhs::math;

namespace boomhs
{

glm::mat4
Transform::model_matrix() const
{
  return math::compute_modelmatrix(translation, rotation, scale);
}

glm::vec3
Transform::get_rotation_radians() const
{
  return glm::eulerAngles(rotation);
}

void
Transform::rotate_radians(float const radians, EulerAxis const axis)
{
  auto& r = rotation;
  switch (axis) {
    case EulerAxis::X:
      r = glm::angleAxis(radians, constants::X_UNIT_VECTOR) * r;
      break;
    case EulerAxis::Y:
      r = glm::angleAxis(radians, constants::Y_UNIT_VECTOR) * r;
      break;
    case EulerAxis::Z:
      r = glm::angleAxis(radians, constants::Z_UNIT_VECTOR) * r;
      break;
    case EulerAxis::INVALID:
      std::abort();
      break;
  }
}

void
Transform::rotate_xyz_radians(glm::vec3 const& rot)
{
  rotate_radians(rot.x, EulerAxis::X);
  rotate_radians(rot.y, EulerAxis::Y);
  rotate_radians(rot.z, EulerAxis::Z);
}

glm::vec3
Transform::get_rotation_degrees() const
{
  return glm::degrees(get_rotation_radians());
}

void
Transform::rotate_degrees(float const degrees, EulerAxis const axis)
{
  rotate_radians(glm::radians(degrees), axis);
}

void
Transform::rotate_xyz_degrees(glm::vec3 const& rot)
{
  rotate_xyz_radians(glm::radians(rot));
}

} // namespace boomhs
