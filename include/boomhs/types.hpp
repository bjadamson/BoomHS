#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace boomhs
{

// tags
struct vertex_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 4; };
struct normal_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 3; };
struct color_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 4; };
struct uv_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 2; };
struct wireframe_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 0; };
struct none_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 0; };

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
