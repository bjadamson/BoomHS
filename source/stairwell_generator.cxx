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
should_skip_tile(int const num_to_place, int const num_placed, TileMap const& tmap,
    TilePosition const& pos, stlw::float_generator &rng)
{
  if(num_placed >= num_to_place) {
    //std::cerr << "placed too many stairs\n";
    // placed too many
    return true;
  }
  if (tmap.data(pos).is_stair()) {
    //std::cerr << "tile already stair\n";
    // tile is already a stairwell
    return true;
  }
  if(any_tilemap_neighbors(tmap, pos, MIN_DISTANCE_BETWEEN_STAIRS, is_stair)) {
    //std::cerr << "too close stair neighbor\n";
    // nearby neighbor tile is a stairwell
    return true;
  }
  {
    auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;
    auto const neighbors = find_immediate_neighbors(tmap, pos, TileType::WALL, behavior);
    auto const ncount = neighbors.size();
    if (ncount < 3) {
      return true;
    }
  }
  {
    auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;
    auto const neighbors = find_immediate_neighbors(tmap, pos, TileType::FLOOR, behavior);
    auto const ncount = neighbors.size();
    if (ncount < 1) {
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
place_stairs(StairGenConfig const& sc, TileMap &tmap, stlw::float_generator &rng,
    entt::DefaultRegistry &registry)
{
  // clang-format off
  int const floor_number     = sc.floor_number;
  int const floor_count      = sc.floor_count;
  int const stairs_perfloor  = sc.stairs_perfloor;

  bool const is_bottom_floor = 0 == floor_number;
  bool const is_top_floor    = (floor_number == (floor_count - 1));

  int upstairs_to_place   = is_top_floor    ? 0 : stairs_perfloor;
  int downstairs_to_place = is_bottom_floor ? 0 : stairs_perfloor;

  int const num_to_place     = upstairs_to_place + downstairs_to_place;
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
    if (is_bottom_floor) {
      return UP();
    }
    else if (is_top_floor) {
      return DOWN();
    }
    else {
      bool const can_place_up = upstairs_to_place > 0;
      bool const can_place_down = downstairs_to_place > 0;
      if (can_place_up && !can_place_down) {
        return UP();
      }
      else if (!can_place_up && can_place_down) {
        return DOWN();
      }
      assert(can_place_up && can_place_down);
      bool const place_up = rng.gen_bool();
      return place_up ? UP() : DOWN();
    }
  };
  int num_placed = 0;
  auto const find_stairpositions = [&](auto const& tpos) {
    if (should_skip_tile(num_to_place, num_placed, tmap, tpos, rng)) {
      //std::cerr << "(floor '" << floor_number << "/" << (floor_count-1) << "' skipping\n";
      return;
    }
    auto &tile = tmap.data(tpos);
    tile.type = calculate_direction();

    auto &si = registry.assign<StairInfo>(tile.eid);
    si.tile_position = tpos;
    ++num_placed;
  };

  while(num_placed < num_to_place) {
    auto const num_remaining = (upstairs_to_place + downstairs_to_place);
    assert((num_remaining + num_placed) == num_to_place);
    tmap.visit_each(find_stairpositions);
  }
  if (num_placed < stairs_perfloor) {
    return false;
  }
  assert(num_placed == num_to_place);
  return true;
}

} // ns boomhs::stairwell_generator
