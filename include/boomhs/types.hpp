#pragma once
#include <stlw/math.hpp>
#include <array>

namespace boomhs
{

struct Transform
{
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  glm::quat rotation;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};

  glm::mat4
  model_matrix() const
  {
    return stlw::math::calculate_modelmatrix(translation, rotation, scale);
  }
};

struct WidthHeightLength
{
  float const width;
  float const height;
  float const length;
};

} // ns opengl
