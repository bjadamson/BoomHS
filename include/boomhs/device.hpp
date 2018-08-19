#pragma once

namespace boomhs
{

struct DeviceSensitivity
{
  float x, y;

  explicit DeviceSensitivity(float const xp, float const yp)
      : x(xp)
      , y(yp)
  {
  }
};

} // namespace boomhs
