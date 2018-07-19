#pragma once
#include <opengl/colors.hpp>

namespace boomhs
{
struct Fog
{
  float density;
  float gradient;

  opengl::Color color;
};

} // namespace boomhs
