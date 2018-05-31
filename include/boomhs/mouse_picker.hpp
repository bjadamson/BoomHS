#pragma once
#include <extlibs/glm.hpp>

namespace boomhs
{
class FrameState;

class MousePicker
{
public:
  explicit MousePicker();
  glm::vec3 calculate_ray(FrameState&) const;
};

} // namespace boomhs
