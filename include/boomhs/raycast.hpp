#pragma once
#include <extlibs/glm.hpp>

namespace boomhs
{
class FrameState;

struct Raycast
{
  Raycast() = delete;

  static glm::vec3 calculate_ray(FrameState&);
};

} // namespace boomhs
