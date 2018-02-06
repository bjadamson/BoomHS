#include <boomhs/stairwell_generator.hpp>
#include <boomhs/level_generator.hpp>
#include <boomhs/tilemap_algorithms.hpp>
#include <stlw/random.hpp>

using namespace boomhs;
static constexpr auto MIN_DISTANCE_BETWEEN_STAIRS = 2;

namespace
{

bool
is_stair(Tile const& tile)
{
  return tile.is_stair();
}

bool
is_floor(Tile const& tile)
{
  return tile.type == TileType::FLOOR;
}

bool
should_skip_tile(int const stairs_perfloor, int const num_placed, TileMap const& tmap,
    TilePosition const& pos, stlw::float_generator &rng)
{
  if(num_placed >= stairs_perfloor) {
    std::cerr << "placed too many stairs\n";
    // placed too many
    return true;
  }
  if (tmap.data(pos).is_stair()) {
    std::cerr << "tile already stair\n";
    // tile is already a stairwell
    return true;
  }
  if(any_tilemap_neighbors(tmap, pos, MIN_DISTANCE_BETWEEN_STAIRS, is_stair)) {
    std::cerr << "too close stair neighbor\n";
    // nearby neighbor tile is a stairwell
    return true;
  }
  if (!any_tilemap_neighbors(tmap, pos, 2, is_floor)) {
    // nearby there must be atleast one floor tile
    std::cerr << "!atleast one floor neighbor\n";
    return true;
  }
  {
    auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;
    auto const neighbors = find_immediate_neighbors(tmap, pos, TileType::FLOOR, TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY);
    auto const ncount = neighbors.size();
    if (ncount < 1 || ncount > 3) {
      return true;
    }
  }
  // Make it increasingly more random which tile's get selected to be stairs, depending on how
  // many stairs have been placed previously.
  FORI(i, num_placed) {
    if (!rng.gen_bool()) {
      std::cerr << "rng failed\n";
      return true;
    }
  }
  return false;
}

} // ns anon

namespace boomhs::stairwell_generator
{

bool
place_stairs(PlaceStairsState &ps, TileMap &tmap, stlw::float_generator &rng,
    entt::DefaultRegistry &registry)
{
  // clang-format off
  auto const& stairconfig    = ps.stairconfig;
  int const floor_number     = stairconfig.floor_number;
  int const floor_count      = stairconfig.floor_count;
  int const stairs_perfloor  = stairconfig.stairs_perfloor;

  int upstairs_to_place   = ps.num_upstairs;
  int downstairs_to_place = ps.num_downstairs;
  assert(stairs_perfloor == (upstairs_to_place + downstairs_to_place));
  // clang-format on

  auto const calculate_direction = [&]()
  {
    auto const UP = [&]() {
      upstairs_to_place--;
      return TileType::STAIR_UP;
    };
    auto const DOWN = [&]() {
      downstairs_to_place--;
      return TileType::STAIR_DOWN;
    };

    bool const is_bottom_floor = 0 == stairconfig.floor_number;
    bool const is_top_floor    = (stairconfig.floor_number == (stairconfig.floor_count - 1));
    if (is_bottom_floor) {
      return UP();
    }
    else if (is_top_floor) {
      return DOWN();
    }
    else {
      if (upstairs_to_place > 0) {
        return UP();
      } else if (downstairs_to_place > 0) {
        return DOWN();
      } else {
        // We should not have any more stairs to be placing at this point.
        std::abort();
      }
    }
  };
  int num_placed = 0;
  auto const find_stairpositions = [&](auto const& pos) {
    if (should_skip_tile(stairs_perfloor, num_placed, tmap, pos, rng)) {
      //std::cerr << "(floor '" << floor_number << "/" << (floor_count-1) << "' skipping\n";
      return;
    }
    auto &tile = tmap.data(pos);
    tile.type = calculate_direction();

    auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;
    auto const neighbors = find_immediate_neighbors(tmap, pos, TileType::FLOOR, behavior);
    std::cerr << "neighbors size: '" << neighbors.size() << "'\n";
    assert(!neighbors.empty());

    auto &si = registry.assign<StairInfo>(tile.eid);
    si.tile_position = pos;
    si.exit_position = neighbors.front();
    ++num_placed;
  };

  while(num_placed < stairs_perfloor) {
    auto const num_remaining = (upstairs_to_place + downstairs_to_place);
    assert((num_remaining + num_placed) == stairs_perfloor);
    tmap.visit_each(find_stairpositions);
  }
  if (num_placed < stairs_perfloor) {
    return false;
  }
  assert(num_placed == stairs_perfloor);
  return true;
}

} // ns boomhs::stairwell_generator
