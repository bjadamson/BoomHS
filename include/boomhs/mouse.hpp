#pragma once
#include <common/type_macros.hpp>
#include <extlibs/sdl.hpp>

namespace boomhs
{

struct ScreenCoordinates
{
  int x, y;
};

struct MouseSensitivity
{
  float x, y;

  explicit MouseSensitivity(float const xp, float const yp)
      : x(xp)
      , y(yp)
  {
  }
};

} // namespace boomhs
