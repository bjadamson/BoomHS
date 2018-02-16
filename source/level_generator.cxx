#include <boomhs/level_generator.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/river_generator.hpp>
#include <boomhs/tiledata.hpp>
#include <boomhs/tiledata_algorithms.hpp>

#include <stlw/optional.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>
#include <algorithm>
#include <vector>
#include <utility>
#include <iostream>

namespace
{

using namespace boomhs;

static auto constexpr ROOM_MAX_SIZE = 5ul;
static auto constexpr ROOM_MIN_SIZE = 3ul;
static auto constexpr MAX_ROOMS = 30;
static auto constexpr MAX_ROOM_MONSTERS = 3;

struct RectCenter
{
  uint64_t x = 0, y = 0;
};

struct Rect
{
  uint64_t x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  MOVE_DEFAULT(Rect);
  COPY_DEFAULT(Rect);

  Rect(uint64_t const x, uint64_t const y, uint64_t const w, uint64_t const h)
    : x1(x)
    , y1(y)
    , x2(x + w)
    , y2(y + h)
  {
  }

  RectCenter
  center() const
  {
    auto const center_x = (x1 + x2) / 2;
    auto const center_y = (y1 + y2) / 2;
    return RectCenter{center_x, center_y};
  }

  bool
  intersects_with(Rect const& other) const
  {
    // returns true if this rectangle intersects with another one
    return (x1 <= other.x2) && (x2 >= other.x1) && (y1 <= other.y2) &&
      (y2 >= other.y1);
  }

  bool
  in_tiledata(TileData const& tiledata) const
  {
    auto const [w, h] = tiledata.dimensions();
    return (x1 > 0) && (y1 > 0) && (x2 < w) && (y2 < h);
  }

  bool
  any_tiles_of_type(TileData const& tiledata, TileType const type) const
  {
    bool any = false;
    for(auto x = x1 + 1; x < x2; ++x) {
      for (auto y = y1 + 1; y < y2; ++y) {
        any |= (tiledata.data(x, y).type == type);
      }
      if (any) {
        break;
      }
    }
    return any;
  }
};

struct RoomGenConfig
{
  uint64_t const tilemap_width;
  uint64_t const tilemap_height;
  std::vector<Rect> const& rooms;
};

std::optional<Rect>
try_create_room(RoomGenConfig const& rgconfig, TileType const type, TileData &tiledata,
    stlw::float_generator &rng)
{
  // random width and height
  auto const w = rng.gen_uint64_range(ROOM_MIN_SIZE, ROOM_MAX_SIZE - 1);
  auto const h = rng.gen_uint64_range(ROOM_MIN_SIZE, ROOM_MAX_SIZE - 1);

  // random position without going out of the boundaries of the map
  auto const xr = rng.gen_uint64_range(0, rgconfig.tilemap_width - w);
  auto const yr = rng.gen_uint64_range(0, rgconfig.tilemap_height - h);
  Rect const new_room{xr, yr, w, h};

  // run through the other rooms and see if they intersect with this one
  for(auto const& r : rgconfig.rooms) {
    // bail early
    if (!new_room.in_tiledata(tiledata) || new_room.intersects_with(r)) {
      return {}; // NONE
    }
  }
  bool const any_river_tiles = new_room.any_tiles_of_type(tiledata, TileType::RIVER);
  bool const any_bridge_tiles = new_room.any_tiles_of_type(tiledata, TileType::BRIDGE);
  if (any_river_tiles || any_bridge_tiles) {
    return {}; // NONE
  }

  for(uint64_t x = new_room.x1 + 1; x < new_room.x2; ++x) {
    for (uint64_t y = new_room.y1 + 1; y < new_room.y2; ++y) {
      tiledata.data(x, y).type = type;
    }
  }
  return new_room;
}

auto
create_room(size_t const max_tries, RoomGenConfig const& rgconfig, TileData &tiledata,
    stlw::float_generator &rng, TileType const type)
{
  assert(rgconfig.tilemap_height - ROOM_MAX_SIZE > 0);
  std::optional<Rect> room;

  size_t trials{0u};
  while(!room && (trials < max_tries)) {
    room = try_create_room(rgconfig, type, tiledata, rng);
    ++trials;
  }
  return room;
}

void
create_h_tunnel(uint64_t const x1, uint64_t const x2, uint64_t const y, TileType const type,
    TileData &tiledata)
{
  uint64_t const min = std::min(x1, x2), max = std::max(x1, x2) + 1;
  for (auto x = min; x <= max; ++x) {
    auto &tile = tiledata.data(x, y);
    if (tile.type == TileType::RIVER) {
      tiledata.assign_bridge(tile);
    }
    else if (tile.type != TileType::BRIDGE) {
      tile.type = type;
    }
  }
}

void
create_v_tunnel(uint64_t const y1, uint64_t const y2, uint64_t const x, TileType const type,
  TileData &tiledata)
{
  uint64_t const min = std::min(y1, y2), max = std::max(y1, y2) + 1;
  for(auto y = min; y <= max; ++y) {
    auto &tile = tiledata.data(x, y);
    if (tile.type == TileType::RIVER) {
      tiledata.assign_bridge(tile);
    }
    else if (tile.type != TileType::BRIDGE) {
      tile.type = type;
    }
  }
}

/*
bool
is_blocked(uint64_t const x, uint64_t const y, TileData const& tiledata)
{
  if (tiledata.data(x, y).type == TileType::WALL) {
    return true;
  }
  return false;
}

auto
generate_monster_position(Rect const& room, TileData const& tiledata, stlw::float_generator &rng)
{
  uint64_t x, y;
  while(true) {
    x = rng.gen_int_range(room.x1 + 1, room.x2);
    y = rng.gen_int_range(room.y1 + 1, room.y2);

    if (!is_blocked(x, y, tiledata)) {
      break;
    }
  }
  return TilePosition{x, y};
}
*/

void
place_objects(Rect const& room, TileData const& tiledata, stlw::float_generator &rng)
{
  auto const num_monsters = rng.gen_int_range(0, MAX_ROOM_MONSTERS + 1);

  FORI(i, num_monsters) {
    //auto const pos = generate_monster_position(room, tiledata, rng);
    if (rng.gen_bool()) {
      // create orc
    } else {
      // generate troll
    }
  }
}

} // ns anon

namespace boomhs::level_generator
{

struct Rooms
{
  std::vector<Rect> rects;
  TilePosition starting_position;

  MOVE_DEFAULT(Rooms);
  NO_COPY(Rooms);
};

std::optional<Rooms>
place_rooms(TileData &tiledata, stlw::float_generator &rng)
{
  auto constexpr MAX_NUM_CREATE_TRIES = 5000;
  std::vector<Rect> rects;
  TilePosition starting_position;

  auto const add_room = [&](auto const& new_room) {
    place_objects(new_room, tiledata, rng);
    rects.emplace_back(new_room);
  };
  auto const connect_rooms = [&rng, &tiledata](auto const prev_center, auto const new_center,
        TileType const type)
  {
    if (rng.gen_bool()) {
      // first move horizontally, then vertically
      create_h_tunnel(prev_center.x, new_center.x, prev_center.y, type, tiledata);
      create_v_tunnel(prev_center.y, new_center.y, new_center.x, type, tiledata);
    } else {
      // first move vertically, then horizontally
      create_v_tunnel(prev_center.y, new_center.y, prev_center.x, type, tiledata);
      create_h_tunnel(prev_center.x, new_center.x, new_center.y, type, tiledata);
    }
  };

  auto const [tdwidth, tdheight] = tiledata.dimensions();
  RoomGenConfig const rgconfig{tdwidth, tdheight, rects};
  {
    // PRESENLTY: algorithm currently assumes we create atleast one room before entering the main
    // loop.
    {
      MAKEOPT(auto const first_room, create_room(MAX_NUM_CREATE_TRIES, rgconfig, tiledata, rng, TileType::FLOOR));
      auto const first_center = first_room.center();
      starting_position.x = first_center.x;
      starting_position.y = first_center.y;
      add_room(first_room);
    }
    assert(rects.size() > 0);
  }

  // Move onto floor tiles.
  assert(!rects.empty());
  FOR(_, MAX_ROOMS) {
    MAKEOPT(auto const new_room, create_room(MAX_NUM_CREATE_TRIES, rgconfig, tiledata, rng,
          TileType::FLOOR));
    // center coordinates of the new room/previous room
    auto const new_center = new_room.center();
    auto const prev_center = rects[rects.size() - 1].center();
    connect_rooms(prev_center, new_center, TileType::FLOOR);
    add_room(new_room);
  }

  assert(!rects.empty());
  return Rooms{MOVE(rects), MOVE(starting_position)};
}

TilePosition
place_rivers_rooms_and_stairs(StairGenConfig const& stairconfig, std::vector<RiverInfo> &rivers,
    TileData &tiledata, stlw::float_generator &rng, entt::DefaultRegistry &registry)
{
  auto const stairs_perfloor = stairconfig.stairs_perfloor;
  assert(stairs_perfloor > 0);

  // 1. Place Rivers
  std::cerr << "placing rivers ...\n";
  river_generator::place_rivers(tiledata, rng, rivers);

  // 2. Place Rooms and Stairs
  std::optional<Rooms> rooms = std::nullopt;
  bool stairs = false;
  while(!rooms && !stairs) {
    std::cerr << "placing rooms ...\n";
    while(!rooms) {
      rooms = place_rooms(tiledata, rng);
    }
    while(!stairs) {
      std::cerr << "placing stairs ...\n";
      stairs = stairwell_generator::place_stairs(stairconfig, tiledata, rng, registry);
    }
  }

  // This seems hacky?
  return (*rooms).starting_position;
}

LevelData
make_leveldata(LevelConfig const& levelconfig, entt::DefaultRegistry &registry,
    TileInfos &&tinfos, stlw::float_generator &rng)
{
  // clang-format off
  TileDataConfig const& tileconfig = levelconfig.tileconfig;
  auto const tdwidth = tileconfig.width;
  auto const tdheight = tileconfig.height;
  auto const num_tiles = tdwidth * tdheight;
  // clang-format on

  std::vector<Tile> tiles{static_cast<size_t>(num_tiles)};
  tiles.reserve(num_tiles);
  TileData tiledata{MOVE(tiles), tdwidth, tdheight, registry};

  std::cerr << "======================================\n";
  std::vector<RiverInfo> rivers;
  auto const starting_pos = place_rivers_rooms_and_stairs(levelconfig.stairconfig, rivers, tiledata,
      rng, registry);
  std::cerr << "======================================\n";

  return LevelData{MOVE(tiledata), MOVE(tinfos), starting_pos, MOVE(rivers)};
}

} // ns boomhs::level_generator
