#pragma once

namespace game
{

struct world_coordinate
{
  float const x, y, z, w;
  constexpr world_coordinate(float const xp, float const yp, float const zp, float const wp)
    : x(xp)
    , y(yp)
    , z(zp)
    , w(wp)
  {
  }
};

} // ns game
