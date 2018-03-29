#pragma once
#include <opengl/colors.hpp>

namespace boomhs
{
struct Fog
{
  float density = 0.0f;
  float gradient = 0.0f;

  opengl::Color color;
};

} // ns boomhs
