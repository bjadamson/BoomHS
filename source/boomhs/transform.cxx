#include <boomhs/transform.hpp>

using namespace boomhs;
using namespace boomhs::math;

namespace boomhs
{

glm::mat4
Transform::model_matrix() const
{
  return math::compute_modelmatrix(translation, rotation_.quat, scale);
}

glm::vec3
Transform::get_rotation_radians() const
{
  return glm::eulerAngles(rotation_.quat);
}

glm::quat
Transform::rotation_quat() const
{
  return rotation_.quat;
}

void
Transform::set_rotation(glm::quat const& rot)
{
  rotation_.quat = rot;
}

void
Transform::rotate_radians(float const radians, EulerAxis const axis)
{
  auto& quat = rotation_.quat;
  switch (axis) {
    case EulerAxis::X:
      quat = glm::angleAxis(radians, constants::X_UNIT_VECTOR) * quat;
      break;
    case EulerAxis::Y:
      quat = glm::angleAxis(radians, constants::Y_UNIT_VECTOR) * quat;
      break;
    case EulerAxis::Z:
      quat = glm::angleAxis(radians, constants::Z_UNIT_VECTOR) * quat;
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
