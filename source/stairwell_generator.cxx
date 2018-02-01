#include <boomhs/stairwell_generator.hpp>
#include <boomhs/tilemap.hpp>
#include <stlw/random.hpp>

using namespace boomhs;
static constexpr auto MAX_NUM_UP_STAIRS_PER_FLOOR = 3;
static constexpr auto MIN_DISTANCE_BETWEEN_STAIRS = 10;

namespace
{

bool
is_stair(Tile const& tile)
{
  return tile.type == TileType::STAIRS;
}

bool
is_floor(Tile const& tile)
{
  return tile.type == TileType::FLOOR;
}

bool
should_skip_tile(TileMap const& tmap, TilePosition const& pos, stlw::float_generator &rng,
    int const num_placed)
{
  if(num_placed >= MAX_NUM_UP_STAIRS_PER_FLOOR) {
    // placed too many
    return true;
  }
  if (tmap.data(pos).type == TileType::STAIRS) {
    // tile is already a stairwell
    return true;
  }
  if(any_tilemap_neighbors(pos, tmap, MIN_DISTANCE_BETWEEN_STAIRS, is_stair)) {
    // nearby neighbor tile is a stairwell
    return true;
  }
  if (!any_tilemap_neighbors(pos, tmap, 2, is_floor)) {
    // nearby there must be atleast one floor tile
    return true;
  }
  {
    auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;
    auto const neighbors = find_neighbor(tmap, pos, TileType::FLOOR, TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY);
    auto const ncount = neighbors.size();
    if (ncount < 1 || ncount > 3) {
      return true;
    }
  }
  // Make it increasingly more random which tile's get selected to be stairs, depending on how
  // many stairs have been placed previously.
  FORI(i, num_placed) {
    if (!rng.gen_bool()) {
      return true;
    }
  }
  return false;
}

} // ns anon

namespace boomhs::stairwell_generator
{

bool
place_stairs(PlaceStairsParams &params)
{
  int const max_tries  = params.max_tries;
  int const num_stairs = params.num_stairs;
  int const max_floors = params.max_floors;
  auto const direction = params.direction;
  auto &tmap           = params.tmap;
  auto &rng            = params.rng;
  auto &registry       = params.registry;

  int num_placed = 0, num_tries = 0;
  auto const find_stairpositions = [&](auto const& pos) {
    if (!should_skip_tile(tmap, pos, rng, num_placed)) {
      auto &tile = tmap.data(pos);
      tile.type = TileType::STAIRS;

      auto &si = registry.assign<StairInfo>(tile.eid);

      auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;
      auto const neighbors = find_neighbor(tmap, pos, TileType::FLOOR, behavior);
      std::cerr << "neigbors size: '" << neighbors.size() << "'\n";
      assert(neighbors.size() > 0);

      TilePosition const stair_exitpos = neighbors[0];

      si.tile_position = pos;
      si.exit_position = glm::vec3{stair_exitpos.x, 0.0, stair_exitpos.z};
      si.direction = direction;
      ++num_placed;

      // since we placed successfully, reset num_tries
      num_tries = 0;
    }
  };
  while(num_placed < num_stairs || num_tries < max_tries) {
    tmap.visit_each(find_stairpositions);
    ++num_tries;
  }
  if (num_placed < num_stairs) {
    return false;
  }
  assert(num_placed == num_stairs);
  return true;
}

} // ns boomhs::stairwell_generator
