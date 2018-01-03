#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    auto const tmatrix = glm::translate(glm::mat4{}, this->translation);
    auto const rmatrix = glm::toMat4(this->rotation);
    auto const smatrix = glm::scale(glm::mat4{}, this->scale);
    return tmatrix * rmatrix * smatrix;
  }
};

struct WidthHeightLength
{
  float const width;
  float const height;
  float const length;
};

} // ns opengl
