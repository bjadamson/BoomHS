#pragma once
#include <extlibs/glm.hpp>
#include <boomhs/math.hpp>

namespace boomhs
{

struct Raycast
{
  Raycast() = delete;

  static glm::vec3 calculate_ray_into_screen(glm::vec2 const&, glm::mat4 const&, glm::mat4 const&, FloatRect const&);
};

} // namespace boomhs
