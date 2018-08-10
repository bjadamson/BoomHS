#pragma once
#include <opengl/colors.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{

struct Fog
{
  float density;
  float gradient;

  opengl::Color color;

  Fog() = default;
  MOVE_CONSTRUCTIBLE_ONLY(Fog);
};

} // namespace boomhs
