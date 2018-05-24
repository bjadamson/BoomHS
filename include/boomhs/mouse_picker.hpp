#pragma once
#include <extlibs/glm.hpp>

namespace boomhs
{
class RenderState;

class MousePicker
{
public:
  explicit MousePicker();
  glm::vec3 calculate_ray(RenderState&) const;
};

} // ns boomhs
