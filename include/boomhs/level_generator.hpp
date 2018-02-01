#pragma once
#include <stlw/random.hpp>
#include <entt/entt.hpp>
#include <boomhs/components.hpp>
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

struct ProcGenState
{
  std::vector<StairDirections> stairs_generated;

  explicit ProcGenState(int const num_up_stairs_per_level)
  {
    FORI(i, num_up_stairs_per_level) {
      stairs_generated.emplace_back(StairDirections::UP);
    }
  }
};

} // ns boomhs

namespace boomhs::level_generator
{

std::pair<TileMap, TilePosition>
make_tilemap(MakeTilemapParams &, ProcGenState &);

} // ns boomhs::level_generator
