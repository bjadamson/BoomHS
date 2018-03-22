#pragma once
#include <array>
#include <stlw/math.hpp>

namespace boomhs
{

struct Transform
{
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  glm::quat rotation;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};

  glm::mat4 model_matrix() const
  {
    return stlw::math::calculate_modelmatrix(translation, rotation, scale);
  }

  void rotate_degrees(float const degrees, glm::vec3 const& axis)
  {
    rotation = glm::angleAxis(glm::radians(degrees), axis) * rotation;
  }
};

struct WidthHeightLength
{
  float const width;
  float const height;
  float const length;
};

} // namespace boomhs
