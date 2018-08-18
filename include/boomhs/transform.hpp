#pragma once
#include <boomhs/math.hpp>

namespace boomhs
{

struct TransformRotation
{
  glm::vec3 euler = math::constants::ZERO;
  glm::quat quat;
};

class Transform
{
  TransformRotation rotation_;

public:
  // fields
  glm::vec3 translation = math::constants::ZERO;
  glm::vec3 scale       = math::constants::ONE;

  // methods
  glm::mat4 model_matrix() const;

  // This uses radians internally.
  glm::quat rotation_quat() const;

  // radians.
  glm::vec3 get_rotation_radians() const;

  void rotate_radians(float, math::EulerAxis);
  void rotate_xyz_radians(glm::vec3 const&);
  void rotate_xyz_radians(float const x, float const y, float const z)
  {
    rotate_xyz_radians(glm::vec3{x, y, z});
  }

  // degrees.
  glm::vec3 get_rotation_degrees() const;

  void rotate_degrees(float, math::EulerAxis);
  void rotate_xyz_degrees(glm::vec3 const&);
  void rotate_xyz_degrees(float const x, float const y, float const z)
  {
    rotate_xyz_degrees(glm::vec3{x, y, z});
  }

  void set_rotation(glm::quat const&);
};

} // namespace boomhs
