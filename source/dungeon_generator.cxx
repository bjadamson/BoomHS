#include <boomhs/dungeon_generator.hpp>
#include <boomhs/enemy.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/river_generator.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>

#include <stlw/optional.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <algorithm>
#include <vector>
#include <utility>
#include <iostream>

namespace
{

using namespace boomhs;
using namespace opengl;

static auto constexpr ROOM_MAX_SIZE = 5ul;
static auto constexpr ROOM_MIN_SIZE = 3ul;
static auto constexpr MAX_ROOMS = 30;

static auto constexpr MIN_MONSTERS_PER_FLOOR = 15;
static auto constexpr MAX_MONSTERS_PER_FLOOR = 30;

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
  in_tilegrid(TileGrid const& tilegrid) const
  {
    auto const [w, h] = tilegrid.dimensions();
    return (x1 > 0) && (y1 > 0) && (x2 < w) && (y2 < h);
  }

  bool
  any_tiles_of_type(TileGrid const& tilegrid, TileType const type) const
  {
    bool any = false;
    for(auto x = x1 + 1; x < x2; ++x) {
      for (auto y = y1 + 1; y < y2; ++y) {
        any |= (tilegrid.data(x, y).type == type);
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
try_create_room(RoomGenConfig const& rgconfig, TileType const type, TileGrid &tilegrid,
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
    if (!new_room.in_tilegrid(tilegrid) || new_room.intersects_with(r)) {
      return {}; // NONE
    }
  }
  bool const any_river_tiles = new_room.any_tiles_of_type(tilegrid, TileType::RIVER);
  bool const any_bridge_tiles = new_room.any_tiles_of_type(tilegrid, TileType::BRIDGE);
  if (any_river_tiles || any_bridge_tiles) {
    return {}; // NONE
  }

  for(uint64_t x = new_room.x1 + 1; x < new_room.x2; ++x) {
    for (uint64_t y = new_room.y1 + 1; y < new_room.y2; ++y) {
      tilegrid.data(x, y).type = type;
    }
  }
  return new_room;
}

auto
create_room(size_t const max_tries, RoomGenConfig const& rgconfig, TileGrid &tilegrid,
    stlw::float_generator &rng, TileType const type)
{
  assert(rgconfig.tilemap_height - ROOM_MAX_SIZE > 0);
  std::optional<Rect> room;

  size_t trials{0u};
  while(!room && (trials < max_tries)) {
    room = try_create_room(rgconfig, type, tilegrid, rng);
    ++trials;
  }
  return room;
}

void
create_h_tunnel(uint64_t const x1, uint64_t const x2, uint64_t const y, TileType const type,
    TileGrid &tilegrid)
{
  uint64_t const min = std::min(x1, x2), max = std::max(x1, x2) + 1;
  for (auto x = min; x <= max; ++x) {
    auto &tile = tilegrid.data(x, y);
    if (tile.type == TileType::RIVER) {
      tilegrid.assign_bridge(tile);
    }
    else if (tile.type != TileType::BRIDGE) {
      tile.type = type;
    }
  }
}

void
create_v_tunnel(uint64_t const y1, uint64_t const y2, uint64_t const x, TileType const type,
  TileGrid &tilegrid)
{
  uint64_t const min = std::min(y1, y2), max = std::max(y1, y2) + 1;
  for(auto y = min; y <= max; ++y) {
    auto &tile = tilegrid.data(x, y);
    if (tile.type == TileType::RIVER) {
      tilegrid.assign_bridge(tile);
    }
    else if (tile.type != TileType::BRIDGE) {
      tile.type = type;
    }
  }
}

bool
is_blocked(uint64_t const x, uint64_t const y, TileGrid const& tilegrid)
{
  auto const type = tilegrid.data(x, y).type;
  if (ANYOF(type == TileType::WALL, type == TileType::STAIR_DOWN, type == TileType::STAIR_UP)) {
    return true;
  }
  return false;
}

auto
generate_monster_position(TileGrid const& tilegrid, EntityRegistry &registry,
    stlw::float_generator &rng)
{
  auto const dimensions = tilegrid.dimensions();
  auto const width = dimensions[0];
  auto const height = dimensions[1];
  assert(width > 0 && height > 0);
  uint64_t x, y;
  while(true) {
    x = rng.gen_int_range(0, width - 1);
    y = rng.gen_int_range(0, height - 1);

    if (is_blocked(x, y, tilegrid)) {
      continue;
    }

    glm::vec3 const pos{x, 0, y};
    static auto constexpr MAX_DISTANCE = 2.0f;
    auto const nearby = all_nearby_entities(pos, MAX_DISTANCE, registry);
    if (!nearby.empty()) {
      continue;
    }
    return TilePosition{x, y};
  }

  std::abort();
  return TilePosition{0, 0};
}

void
place_monsters(TileGrid const& tilegrid, EntityRegistry &registry, stlw::float_generator &rng)
{
  auto const num_monsters = rng.gen_int_range(MIN_MONSTERS_PER_FLOOR, MAX_MONSTERS_PER_FLOOR);

  auto const make_monster = [&](char const* name) {
    auto const tpos = generate_monster_position(tilegrid, registry, rng);
    Enemy::load_new(registry, name, tpos);
  };

  FORI(i, num_monsters) {
    if (rng.gen_bool()) {
      make_monster("O");
    } else {
      make_monster("T");
    }
  }
}

EntityID
place_torch(TileGrid const& tilegrid, EntityRegistry &registry, stlw::float_generator &rng)
{
  auto eid = registry.create();
  registry.assign<Torch>(eid);

  auto &isv = registry.assign<IsVisible>(eid);
  isv.value = true;

  auto &pointlight = registry.assign<PointLight>(eid);
  pointlight.light.diffuse = LOC::YELLOW;

  auto &flicker = registry.assign<LightFlicker>(eid);
  flicker.base_speed = 1.0f;
  flicker.current_speed = flicker.base_speed;

  flicker.colors[0] = LOC::RED;
  flicker.colors[1] = LOC::YELLOW;

  auto &att = pointlight.attenuation;
  att.constant = 1.0f;
  att.linear = 0.93f;
  att.quadratic = 0.46f;

  auto &torch_transform = registry.assign<Transform>(eid);

  auto const pos = generate_monster_position(tilegrid, registry, rng);
  torch_transform.translation = glm::vec3{pos.x, 0.5, pos.y};
  std::cerr << "torchlight pos: '" << torch_transform.translation << "'\n";

  auto &mesh = registry.assign<MeshRenderable>(eid);
  mesh.name = "O_no_normals";

  auto &sn = registry.assign<ShaderName>(eid);
  sn.value = "light";

  return eid;
}

} // ns anon

namespace boomhs::dungeon_generator
{

struct Rooms
{
  std::vector<Rect> rects;
  TilePosition starting_position;

  MOVE_DEFAULT(Rooms);
  NO_COPY(Rooms);
};

std::optional<Rooms>
place_rooms(TileGrid &tilegrid, stlw::float_generator &rng)
{
  auto constexpr MAX_NUM_CREATE_TRIES = 5000;
  std::vector<Rect> rects;
  TilePosition starting_position;

  auto const add_room = [&](auto const& new_room) {
    rects.emplace_back(new_room);
  };
  auto const connect_rooms = [&rng, &tilegrid](auto const prev_center, auto const new_center,
        TileType const type)
  {
    if (rng.gen_bool()) {
      // first move horizontally, then vertically
      create_h_tunnel(prev_center.x, new_center.x, prev_center.y, type, tilegrid);
      create_v_tunnel(prev_center.y, new_center.y, new_center.x, type, tilegrid);
    } else {
      // first move vertically, then horizontally
      create_v_tunnel(prev_center.y, new_center.y, prev_center.x, type, tilegrid);
      create_h_tunnel(prev_center.x, new_center.x, new_center.y, type, tilegrid);
    }
  };

  auto const [tdwidth, tdheight] = tilegrid.dimensions();
  RoomGenConfig const rgconfig{tdwidth, tdheight, rects};
  {
    // PRESENLTY: algorithm currently assumes we create atleast one room before entering the main
    // loop.
    {
      MAKEOPT(auto const first_room, create_room(MAX_NUM_CREATE_TRIES, rgconfig, tilegrid, rng, TileType::FLOOR));
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
    MAKEOPT(auto const new_room, create_room(MAX_NUM_CREATE_TRIES, rgconfig, tilegrid, rng,
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
    TileGrid &tilegrid, stlw::float_generator &rng, EntityRegistry &registry)
{
  auto const stairs_perfloor = stairconfig.stairs_perfloor;
  assert(stairs_perfloor > 0);

  // 1. Place Rivers
  std::cerr << "placing rivers ...\n";
  RiverGenerator::place_rivers(tilegrid, rng, rivers);

  // 2. Place Rooms and Stairs
  std::optional<Rooms> rooms = std::nullopt;
  bool stairs = false;
  while(!rooms && !stairs) {
    std::cerr << "placing rooms ...\n";
    while(!rooms) {
      rooms = place_rooms(tilegrid, rng);
    }
    if (1 == stairconfig.floor_count) {
      std::cerr << "one floor, skipping placing stairs ...\n";
      break;
    }
    while(!stairs) {
      std::cerr << "placing stairs ...\n";
      stairs = stairwell_generator::place_stairs(stairconfig, tilegrid, rng, registry);
    }
  }

  std::cerr << "placing monsters ...\n";
  place_monsters(tilegrid, registry, rng);

  // This seems hacky?
  return (*rooms).starting_position;
}

LevelGeneredData
gen_level(LevelConfig const& levelconfig, EntityRegistry &registry, stlw::float_generator &rng)
{
  // clang-format off
  TileGridConfig const& tileconfig = levelconfig.tileconfig;
  auto const tdwidth = tileconfig.width;
  auto const tdheight = tileconfig.height;
  auto const num_tiles = tdwidth * tdheight;
  // clang-format on

  TileGrid tilegrid{tdwidth, tdheight, registry};
  floodfill(tilegrid, TileType::WALL);

  std::cerr << "======================================\n";
  std::vector<RiverInfo> rivers;
  auto const starting_pos = place_rivers_rooms_and_stairs(levelconfig.stairconfig, rivers, tilegrid,
      rng, registry);

  std::cerr << "placing torch ...\n";
  auto const torch_eid = place_torch(tilegrid, registry, rng);

  std::cerr << "finished!\n";
  std::cerr << "======================================\n";

  return LevelGeneredData{MOVE(tilegrid), starting_pos, MOVE(rivers), torch_eid};
}

} // ns boomhs::dungeon_generator
