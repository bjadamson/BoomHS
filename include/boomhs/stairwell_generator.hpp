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
class TileMap;
} // ns boomhs

namespace boomhs::stairwell_generator
{

struct PlaceStairsParams
{
  int const max_tries, max_floors, num_stairs;
  StairDirections const direction;

  TileMap &tmap;
  stlw::float_generator &rng;
  entt::DefaultRegistry &registry;
};

bool
place_stairs(PlaceStairsParams &);

} // ns boomhs::stairwell_generator
