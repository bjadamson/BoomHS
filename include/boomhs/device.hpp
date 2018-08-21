#pragma once

namespace boomhs
{

struct DeviceSensitivity
{
  float x = 1.0f, y = 1.0f;

  DeviceSensitivity() = default;
  explicit DeviceSensitivity(float const xp, float const yp)
      : x(xp)
      , y(yp)
  {
  }
};

} // namespace boomhs
