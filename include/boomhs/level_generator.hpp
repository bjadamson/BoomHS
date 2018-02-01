#pragma once
#include <stlw/random.hpp>
#include <entt/entt.hpp>
#include <boomhs/tilemap.hpp>

namespace boomhs
{

struct MakeTilemapParams
{
  int const width, length, num_floors;
  int const floor_number;

  stlw::float_generator &rng;
  entt::DefaultRegistry &registry;
};

} // ns boomhs

namespace boomhs::level_generator
{

std::pair<TileMap, TilePosition>
make_tilemap(MakeTilemapParams &);

} // ns boomhs::level_generator
