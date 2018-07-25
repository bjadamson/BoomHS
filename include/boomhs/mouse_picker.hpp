#pragma once
#include <extlibs/glm.hpp>

namespace opengl
{
class FrameState;
} // namespace opengl

namespace boomhs
{
class MousePicker
{
public:
  explicit MousePicker();
  glm::vec3 calculate_ray(opengl::FrameState&) const;
};

} // namespace boomhs
