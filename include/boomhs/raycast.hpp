#pragma once
#include <extlibs/glm.hpp>

namespace boomhs
{
class Viewport;

struct Raycast
{
  Raycast() = delete;

  static glm::vec3
  calculate_ray_into_screen(glm::vec2 const&, glm::mat4 const&, glm::mat4 const&, Viewport const&);
};

} // namespace boomhs
