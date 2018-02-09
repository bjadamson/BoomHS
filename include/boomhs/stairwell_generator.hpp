#pragma once
#include <boomhs/components.hpp>
#include <stlw/type_macros.hpp>
#include <entt/entt.hpp>
#include <iostream>

namespace stlw
{
class float_generator;
} // ns stlw

namespace boomhs
{
class TileData;

struct StairGenConfig
{
  int const floor_count;
  int const floor_number;
  int const stairs_perfloor;

  NO_COPY_AND_NO_MOVE(StairGenConfig);
};

} // ns boomhs

namespace boomhs::stairwell_generator
{

bool
place_stairs(StairGenConfig const&, TileData &, stlw::float_generator &, entt::DefaultRegistry &);

} // ns boomhs::stairwell_generator
