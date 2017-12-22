#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace boomhs
{

// tags
struct vertex_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 4; };
struct normal_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 3; };
struct color_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 4; };
struct uv_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 2; };
struct wireframe_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 0; };

struct Transform
{
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  glm::quat rotation;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};
};

struct WidthHeightLength
{
  float const width;
  float const height;
  float const length;
};

} // ns opengl
