#pragma once
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace stlw
{
class float_generator;
} // namespace stlw

namespace boomhs
{
class EntityRegistry;
class TileGrid;

struct StairGenConfig
{
  int const floor_count;
  int const floor_number;
  int const stairs_perfloor;

  NO_COPYMOVE(StairGenConfig);
};

} // namespace boomhs

namespace boomhs::stairwell_generator
{

bool
place_stairs(stlw::Logger&, StairGenConfig const&, TileGrid&, stlw::float_generator&,
             EntityRegistry&);

} // namespace boomhs::stairwell_generator
