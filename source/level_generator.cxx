#include <boomhs/level_generator.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/leveldata.hpp>
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
  in_tiledata(TileData const& tdata) const
  {
    auto const [w, h] = tdata.dimensions();
    return (x1 > 0) && (y1 > 0) && (x2 < w) && (y2 < h);
  }

  bool
  any_tiles_of_type(TileData const& tdata, TileType const type) const
  {
    bool any = false;
    for(auto x = x1 + 1; x < x2; ++x) {
      for (auto y = y1 + 1; y < y2; ++y) {
        any |= (tdata.data(x, y).type == type);
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

stlw::optional<Rect>
try_create_room(RoomGenConfig const& rgconfig, TileType const type, TileData &tdata,
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
    if (!new_room.in_tiledata(tdata) || new_room.intersects_with(r)) {
      return {}; // NONE
    }
  }
  bool const any_river_tiles = new_room.any_tiles_of_type(tdata, TileType::RIVER);
  bool const any_bridge_tiles = new_room.any_tiles_of_type(tdata, TileType::BRIDGE);
  if (any_river_tiles || any_bridge_tiles) {
    return {}; // NONE
  }

  for(uint64_t x = new_room.x1 + 1; x < new_room.x2; ++x) {
    for (uint64_t y = new_room.y1 + 1; y < new_room.y2; ++y) {
      tdata.data(x, y).type = type;
    }
  }
  return new_room;
}

auto
create_room(size_t const max_tries, RoomGenConfig const& rgconfig, TileData &tdata,
    stlw::float_generator &rng, TileType const type)
{
  assert(rgconfig.tilemap_height - ROOM_MAX_SIZE > 0);
  stlw::optional<Rect> room;

  std::size_t trials{0u};
  while(!room && (trials < max_tries)) {
    room = try_create_room(rgconfig, type, tdata, rng);
    ++trials;
  }
  return room;
}

void
create_h_tunnel(uint64_t const x1, uint64_t const x2, uint64_t const y, TileType const type, TileData &tdata)
{
  uint64_t const min = std::min(x1, x2), max = std::max(x1, x2) + 1;
  for (auto x = min; x <= max; ++x) {
    auto &tile = tdata.data(x, y);
    if (ANYOF(tile.type == TileType::RIVER, tile.type == TileType::BRIDGE)) {
      tile.type = TileType::BRIDGE;
    }
    else {
      tile.type = type;
    }
  }
}

void
create_v_tunnel(uint64_t const y1, uint64_t const y2, uint64_t const x, TileType const type, TileData &tdata)
{
  uint64_t const min = std::min(y1, y2), max = std::max(y1, y2) + 1;
  for(auto y = min; y <= max; ++y) {
    auto &tile = tdata.data(x, y);
    if (ANYOF(tile.type == TileType::RIVER, tile.type == TileType::BRIDGE)) {
      tile.type = TileType::BRIDGE;
    }
    else {
      tile.type = type;
    }
  }
}

/*
bool
is_blocked(uint64_t const x, uint64_t const y, TileData const& tdata)
{
  if (tdata.data(x, y).type == TileType::WALL) {
    return true;
  }
  return false;
}

auto
generate_monster_position(Rect const& room, TileData const& tdata, stlw::float_generator &rng)
{
  uint64_t x, y;
  while(true) {
    x = rng.gen_int_range(room.x1 + 1, room.x2);
    y = rng.gen_int_range(room.y1 + 1, room.y2);

    if (!is_blocked(x, y, tdata)) {
      break;
    }
  }
  return TilePosition{x, y};
}
*/

void
place_objects(Rect const& room, TileData const& tdata, stlw::float_generator &rng)
{
  auto const num_monsters = rng.gen_int_range(0, MAX_ROOM_MONSTERS + 1);

  FORI(i, num_monsters) {
    //auto const pos = generate_monster_position(room, tdata, rng);
    if (rng.gen_bool()) {
      // create orc
    } else {
      // generate troll
    }
  }
}

Edges
TEST(TilePosition const& tpos, size_t const tdata_width, size_t const tdata_height,
    size_t const distance_v, size_t const distance_h)
{
  assert(tdata_width > 0ul);
  assert(tdata_height > 0ul);

  auto const left   = tpos.x > distance_v ? std::max(tpos.x - distance_v, 0ul) : 0ul;
  auto const right  = std::min(tpos.x + distance_v, tdata_width - 1);

  auto const top    = std::min(tpos.y + distance_h, tdata_height - 1);
  auto const bottom = tpos.y > distance_h ? std::max(tpos.y - distance_h, 0ul) : 0ul;

  //std::cerr << "left: '" << left << "' right: '" << right << "'\n";
  assert(left <= right);
  assert(bottom <= top);

  return Edges{tpos, left, top, right, bottom};
}

stlw::optional<RiverInfo>
generate_river(TileData &tdata, stlw::float_generator &rng)
{
  auto const [tdwidth, tdheight] = tdata.dimensions();
  auto const tpos_edge = random_tileposition_onedgeofmap(tdata, rng);
  auto const& tpos = tpos_edge.first;
  MapEdge const& edge = tpos_edge.second;

  auto const RIVER_DISTANCE = 1;
  if (edge.is_xedge()) {
    FOR(i, tdwidth) {
      tdata.data(i, tpos.y).type = TileType::RIVER;
    }

    auto constexpr FLOW_DIR = glm::vec2{1.0, 0.0};
    auto const edges = TEST(tpos, tdwidth, tdheight, tdwidth, RIVER_DISTANCE);
    return RiverInfo{tpos, edges.left, edges.top, edges.right, edges.bottom, FLOW_DIR};
  }
  else {
    assert(edge.is_yedge());
    FOR(i, tdheight) {
      tdata.data(tpos.x, i).type = TileType::RIVER;
    }

    auto constexpr FLOW_DIR = glm::vec2{0.0, 1.0};
    auto const edges = TEST(tpos, tdwidth, tdheight, RIVER_DISTANCE, tdheight);
    return RiverInfo{tpos, edges.left, edges.top, edges.right, edges.bottom, FLOW_DIR};
  }
  std::abort();
  return stlw::none;
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

void
place_rivers(TileData &tdata, stlw::float_generator &rng, std::vector<RiverInfo> &rivers)
{
  stlw::optional<RiverInfo> river_o = stlw::none;
  while(!river_o) {
    river_o = generate_river(tdata, rng);
  }
  assert(river_o);
  auto river = MOVE(*river_o);

  FOR(_, 50) {
    float const speed    = rng.gen_float_range(50.0, 250.0);
    float const z_jiggle = rng.gen_float_range(1000.0, 1500.0f);
    glm::vec2 const wiggle_pos{river.tile_position.x, river.tile_position.y};

    RiverWiggle wiggle{speed, z_jiggle, wiggle_pos, river.flow_direction};
    river.wiggles.emplace_back(MOVE(wiggle));
    rivers.emplace_back(MOVE(river));
  }
}

stlw::optional<Rooms>
create_rooms(TileData &tdata, stlw::float_generator &rng)
{
  auto constexpr MAX_NUM_CREATE_TRIES = 5000;
  std::vector<Rect> rects;
  TilePosition starting_position;

  auto const add_room = [&](auto const& new_room) {
    place_objects(new_room, tdata, rng);
    rects.emplace_back(new_room);
  };
  auto const connect_rooms = [&rng, &tdata](auto const prev_center, auto const new_center,
        TileType const type)
  {
    if (rng.gen_bool()) {
      // first move horizontally, then vertically
      create_h_tunnel(prev_center.x, new_center.x, prev_center.y, type, tdata);
      create_v_tunnel(prev_center.y, new_center.y, new_center.x, type, tdata);
    } else {
      // first move vertically, then horizontally
      create_v_tunnel(prev_center.y, new_center.y, prev_center.x, type, tdata);
      create_h_tunnel(prev_center.x, new_center.x, new_center.y, type, tdata);
    }
  };

  auto const [tdwidth, tdheight] = tdata.dimensions();
  RoomGenConfig const rgconfig{tdwidth, tdheight, rects};
  {
    // PRESENLTY: algorithm currently assumes we create atleast one room before entering the main
    // loop.
    {
      MAKEOPT(auto const first_room, create_room(MAX_NUM_CREATE_TRIES, rgconfig, tdata, rng, TileType::FLOOR));
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
    MAKEOPT(auto const new_room, create_room(MAX_NUM_CREATE_TRIES, rgconfig, tdata, rng,
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
    TileData &tdata, stlw::float_generator &rng, entt::DefaultRegistry &registry)
{
  auto const stairs_perfloor = stairconfig.stairs_perfloor;
  assert(stairs_perfloor > 0);

  // 1. Place Rivers
  place_rivers(tdata, rng, rivers);

  // 2. Place Rooms and Stairs
  stlw::optional<Rooms> rooms = stlw::none;
  bool stairs = false;
  while(!rooms && !stairs) {
    std::cerr << "creating rooms ...\n";
    while(!rooms) {
      rooms = create_rooms(tdata, rng);
    }
    while(!stairs) {
      std::cerr << "placing stairs ...\n";
      stairs = stairwell_generator::place_stairs(stairconfig, tdata, rng, registry);
    }
  }

  // This seems hacky?
  return (*rooms).starting_position;
}

LevelData
make_leveldata(TileDataConfig const& tdconfig, stlw::float_generator &rng,
    entt::DefaultRegistry &registry)
{
  // clang-format off
  auto const tdwidth = tdconfig.width;
  auto const tdheight = tdconfig.height;
  auto const num_tiles = tdwidth * tdheight;
  // clang-format on

  std::vector<Tile> tiles{static_cast<std::size_t>(num_tiles)};
  tiles.reserve(num_tiles);
  TileData tdata{MOVE(tiles), tdwidth, tdheight, registry};

  std::cerr << "======================================\n";
  std::vector<RiverInfo> rivers;
  auto const starting_pos = place_rivers_rooms_and_stairs(tdconfig.stairconfig, rivers, tdata, rng, registry);
  std::cerr << "======================================\n";

  return LevelData{MOVE(tdata), starting_pos, MOVE(rivers)};
}

} // ns boomhs::level_generator
