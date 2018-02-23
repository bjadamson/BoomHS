#pragma once
#include <stlw/type_macros.hpp>
#include <iostream>

namespace stlw
{
class float_generator;
} // ns stlw

namespace boomhs
{
class EntityRegistry;
class TileGrid;

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
place_stairs(StairGenConfig const&, TileGrid &, stlw::float_generator &, EntityRegistry &);

} // ns boomhs::stairwell_generator
