#pragma once
#include <boomhs/math.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{

struct Transform
{
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  glm::quat rotation;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};

  glm::mat4 model_matrix() const
  {
    return math::calculate_modelmatrix(translation, rotation, scale);
  }

  void rotate_degrees(float const degrees, glm::vec3 const& axis)
  {
    rotation = glm::angleAxis(glm::radians(degrees), axis) * rotation;
  }

  void rotate_xyz_degrees(glm::vec3 const& rot)
  {
    rotate_degrees(rot.x, math::constants::X_UNIT_VECTOR);
    rotate_degrees(rot.y, math::constants::Y_UNIT_VECTOR);
    rotate_degrees(rot.z, math::constants::Z_UNIT_VECTOR);
  }
};

} // namespace boomhs
