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
class TileMap;

struct StairGenConfig
{
  int const floor_count;
  int const floor_number;
  int const stairs_perfloor;

  NO_COPY_AND_NO_MOVE(StairGenConfig);
};

struct PlaceStairsState
{
  StairGenConfig const& stairconfig;
  uint32_t const num_upstairs;
  uint32_t const num_downstairs;

  PlaceStairsState(StairGenConfig const& sgc, uint32_t const nu, uint32_t const nd)
    : stairconfig(sgc)
    , num_upstairs(nu)
    , num_downstairs(nd)
  {
  }
  NO_COPY_AND_NO_MOVE(PlaceStairsState);
};

} // ns boomhs

namespace boomhs::stairwell_generator
{

bool
place_stairs(PlaceStairsState &, TileMap &, stlw::float_generator &, entt::DefaultRegistry &);

} // ns boomhs::stairwell_generator
