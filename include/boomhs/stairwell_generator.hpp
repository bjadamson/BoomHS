#pragma once
#include <boomhs/components.hpp>
#include <stlw/type_macros.hpp>
#include <entt/entt.hpp>
#include <vector>

namespace stlw
{
class float_generator;
} // ns stlw

namespace boomhs
{
struct ProcGenState;
class TileMap;
} // ns boomhs

namespace boomhs::stairwell_generator
{

struct PlaceStairsParams
{
  int const floor_number, max_floors, num_stairs;
  StairDirections const direction;

  TileMap &tmap;
  stlw::float_generator &rng;
  entt::DefaultRegistry &registry;
};

bool
place_stairs(PlaceStairsParams &, ProcGenState &);

} // ns boomhs::stairwell_generator
