#include <boomhs/level_generator.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/tiledata.hpp>

#include <stlw/optional.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <algorithm>
#include <stlw/optional.hpp>
#include <iostream>
#include <vector>
#include <utility>

namespace
{

using namespace boomhs;

static auto constexpr ROOM_MAX_SIZE = 5;
static auto constexpr ROOM_MIN_SIZE = 3;
static auto constexpr MAX_ROOMS = 30;
static auto constexpr MAX_ROOM_MONSTERS = 3;

struct RectCenter
{
  int x = 0, y = 0;
};

struct Rect
{
  int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  MOVE_DEFAULT(Rect);
  COPY_DEFAULT(Rect);

  Rect(int const x, int const y, int const w, int const h)
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
    // TODO: dim.l is for our hack, if we start generating in 3D
    auto const [w, l] = tdata.dimensions();
    auto const cast = [](auto const v) { return static_cast<int>(v); };
    return (x1 > 0) && (y1 > 0) && (x2 < cast(w)) && (y2 < cast(l));
  }
};

struct RoomGenConfig
{
  int const width;
  int const height;
  std::vector<Rect> const& rooms;
};

stlw::optional<Rect>
try_create_room(RoomGenConfig const& rgconfig, TileData &tdata, stlw::float_generator &rng,
    TileType const type)
{
  // random width and height
  auto const w = rng.gen_int_range(ROOM_MIN_SIZE, ROOM_MAX_SIZE + 1);
  auto const h = rng.gen_int_range(ROOM_MIN_SIZE, ROOM_MAX_SIZE + 1);

  // random position without going out of the boundaries of the map
  auto const xr = rng.gen_int_range(0, rgconfig.width - w);
  auto const yr = rng.gen_int_range(0, rgconfig.height - h);
  Rect const new_room{xr, yr, w, h};

  // run through the other rooms and see if they intersect with this one
  for(auto const& r : rgconfig.rooms) {
    // bail early
    if (!new_room.in_tiledata(tdata) || new_room.intersects_with(r)) {
      return {}; // NONE
    }
  }
  for(int x = new_room.x1 + 1; x < new_room.x2; ++x) {
    for (int y = new_room.y1 + 1; y < new_room.y2; ++y) {
      tdata.data(x, y).type = type;
    }
  }
  return new_room;
}

auto
create_room(size_t const max_tries, RoomGenConfig const& rgconfig, TileData &tdata,
    stlw::float_generator &rng, TileType const type)
{
  assert(rgconfig.height - ROOM_MAX_SIZE > 0);
  stlw::optional<Rect> room;

  std::size_t trials{0u};
  while(!room && (trials < max_tries)) {
    room = try_create_room(rgconfig, tdata, rng, type);
    ++trials;
  }
  return room;
}

void
create_h_tunnel(int const x1, int const x2, int const y, TileType const type, TileData &tdata)
{
  int const min = std::min(x1, x2), max = std::max(x1, x2) + 1;
  for (auto x = min; x <= max; ++x) {
    tdata.data(x, y).type = type;
  }
}

void
create_v_tunnel(int const y1, int const y2, int const x, TileType const type, TileData &tdata)
{
  int const min = std::min(y1, y2), max = std::max(y1, y2) + 1;
  for(auto y = min; y <= max; ++y) {
    tdata.data(x, y).type = type;
  }
}

bool
is_blocked(int const x, int const y, TileData const& tdata)
{
  if (tdata.data(x, y).type == TileType::WALL) {
    return true;
  }
  return false;
}

auto
generate_monster_position(Rect const& room, TileData const& tdata, stlw::float_generator &rng)
{
  int x, y;
  while(true) {
    x = rng.gen_int_range(room.x1 + 1, room.x2);
    y = rng.gen_int_range(room.y1 + 1, room.y2);

    if (!is_blocked(x, y, tdata)) {
      break;
    }
  }
  return TilePosition{x, y};
}

void
place_objects(Rect const& room, TileData const& tdata, stlw::float_generator &rng)
{
  auto const num_monsters = rng.gen_int_range(0, MAX_ROOM_MONSTERS + 1);

  FORI(i, num_monsters) {
    auto const pos = generate_monster_position(room, tdata, rng);
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

stlw::optional<Rooms>
create_rooms(int const width, int const height, TileData &tdata, stlw::float_generator &rng)
{
  auto constexpr MAX_NUM_CREATE_TRIES = 5000;
  std::vector<Rect> rects;
  TilePosition starting_position;

  auto const add_room = [&](auto const& new_room) {
    place_objects(new_room, tdata, rng);
    rects.emplace_back(new_room);
  };

  // PRESENLTY: algorithm currently assumes we create atleast one room before entering the main
  // loop.
  RoomGenConfig const rgconfig{width, height, rects};
  {
    // 1. Create river room
    {
      MAKEOPT(auto const new_room, create_room(MAX_NUM_CREATE_TRIES, rgconfig, tdata, rng,
            TileType::FLOOR));
      auto const center = new_room.center();
      starting_position.x = center.x;
      starting_position.y = center.y;
      add_room(new_room);
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

    if (rng.gen_bool()) {
      // first move horizontally, then vertically
      create_h_tunnel(prev_center.x, new_center.x, prev_center.y, TileType::FLOOR, tdata);
      create_v_tunnel(prev_center.y, new_center.y, new_center.x, TileType::FLOOR, tdata);
    } else {
      // first move vertically, then horizontally
      create_v_tunnel(prev_center.y, new_center.y, prev_center.x, TileType::FLOOR, tdata);
      create_h_tunnel(prev_center.x, new_center.x, new_center.y, TileType::FLOOR, tdata);
    }
    add_room(new_room);
  }

  assert(!rects.empty());
  return Rooms{MOVE(rects), MOVE(starting_position)};
}

TilePosition
place_rooms_and_stairs(TilemapConfig &tconfig, TileData &tdata,
    stlw::float_generator &rng, entt::DefaultRegistry &registry)
{
  auto const& sc = tconfig.stairconfig;
  auto const stairs_perfloor = sc.stairs_perfloor;
  assert(stairs_perfloor > 0);

  stlw::optional<Rooms> rooms = stlw::none;
  bool stairs = false;

  auto const add_river = [&](auto const x, auto const y) {
    auto &tile = tdata.data(x, y);
    tile.type = TileType::RIVER;

    auto constexpr WIDTH = 5;
    auto constexpr HEIGHT = 5;

    auto &ri = registry.assign<RiverInfo>(tile.eid);
    ri.left     = glm::vec3{x, 0, 0};
    ri.right    = glm::vec3{x + WIDTH, 0, 0};

    ri.top      = glm::vec3{0, 0, y};
    ri.bottom   = glm::vec3{0, 0, y + HEIGHT};

    FOR(i, 1) {
      float const speed    = rng.gen_float_range(50.0, 250.0);
      float const z_jiggle = rng.gen_float_range(1000.0, 1500.0f);
      glm::vec3 const position{x, 0, y + i};

      ri.wiggles.emplace_back(RiverWiggle{speed, z_jiggle, position});
    }
  };

  while(!rooms && !stairs) {
    std::cerr << "creating rooms ...\n";
    while(!rooms) {
      rooms = create_rooms(tconfig.width, tconfig.length, tdata, rng);
    }
    while(!stairs) {
      std::cerr << "placing stairs ...\n";
      stairs = stairwell_generator::place_stairs(sc, tdata, rng, registry);
    }
  }

  // This seems hacky?
  return (*rooms).starting_position;
}

std::pair<TileData, TilePosition>
make_tiledata(TilemapConfig &tconfig, stlw::float_generator &rng, entt::DefaultRegistry &registry)
{
  // clang-format off
  int const width     = tconfig.width;
  int const length    = tconfig.length;
  int const num_tiles = width * length;
  // clang-format on

  std::vector<Tile> tiles{static_cast<std::size_t>(num_tiles)};
  tiles.reserve(width * length);
  TileData tdata{MOVE(tiles), width, length, registry};

  std::cerr << "======================================\n";
  auto starting_pos = place_rooms_and_stairs(tconfig, tdata, rng, registry);
  std::cerr << "======================================\n";

  return std::make_pair(MOVE(tdata), MOVE(starting_pos));
}

} // ns boomhs::level_generator
