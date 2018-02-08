#include <boomhs/level_generator.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/tilemap.hpp>
#include <stlw/random.hpp>
#include <stlw/type_macros.hpp>
#include <algorithm>
#include <boost/optional.hpp>
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
  in_tilemap(TileMap const& tmap) const
  {
    // TODO: dim.l is for our hack, if we start generating in 3D
    auto const [w, l] = tmap.dimensions();
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

boost::optional<Rect>
try_create_room(RoomGenConfig const& rgconfig, TileMap &tmap, stlw::float_generator &rng)
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
    if (!new_room.in_tilemap(tmap) || new_room.intersects_with(r)) {
      return {}; // NONE
    }
  }
  for(int x = new_room.x1 + 1; x < new_room.x2; ++x) {
    for (int y = new_room.y1 + 1; y < new_room.y2; ++y) {
      tmap.data(x, y).type = TileType::FLOOR;
    }
  }
  return new_room;
}

auto
create_room(size_t const max_tries, RoomGenConfig const& rgconfig, TileMap &tmap,
    stlw::float_generator &rng)
{
  assert(rgconfig.height - ROOM_MAX_SIZE > 0);
  boost::optional<Rect> room;

  std::size_t trials{0u};
  while(!room && (trials < max_tries)) {
    room = try_create_room(rgconfig, tmap, rng);
    ++trials;
  }
  return room;
}

void
create_h_tunnel(int const x1, int const x2, int const y, TileMap &tmap)
{
  int const min = std::min(x1, x2), max = std::max(x1, x2) + 1;
  for (auto x = min; x <= max; ++x) {
    tmap.data(x, y).type = TileType::FLOOR;
  }
}

void
create_v_tunnel(int const y1, int const y2, int const x, TileMap &tmap)
{
  int const min = std::min(y1, y2), max = std::max(y1, y2) + 1;
  for(auto y = min; y <= max; ++y) {
    tmap.data(x, y).type = TileType::FLOOR;
  }
}

bool
is_blocked(int const x, int const y, TileMap const& tmap)
{
  if (tmap.data(x, y).type == TileType::WALL) {
    return true;
  }
  return false;
}

auto
generate_monster_position(Rect const& room, TileMap const& tmap, stlw::float_generator &rng)
{
  int x, y;
  while(true) {
    x = rng.gen_int_range(room.x1 + 1, room.x2);
    y = rng.gen_int_range(room.y1 + 1, room.y2);

    if (!is_blocked(x, y, tmap)) {
      break;
    }
  }
  return TilePosition{x, y};
}

void
place_objects(Rect const& room, TileMap const& tmap, stlw::float_generator &rng)
{
  auto const num_monsters = rng.gen_int_range(0, MAX_ROOM_MONSTERS + 1);

  FORI(i, num_monsters) {
    auto const pos = generate_monster_position(room, tmap, rng);
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

boost::optional<Rooms>
create_rooms(int const width, int const height, TileMap &tmap, stlw::float_generator &rng)
{
  auto constexpr MAX_NUM_CREATE_TRIES = 5000;
  std::vector<Rect> rects;
  TilePosition starting_position;

  FOR(_, MAX_ROOMS) {
    RoomGenConfig const rgconfig{width, height, rects};
    auto new_room_o = create_room(MAX_NUM_CREATE_TRIES, rgconfig, tmap, rng);
    if(!new_room_o) {
      return boost::none;
    }
    auto new_room = *new_room_o;
    auto const new_center = new_room.center();

    if (rects.empty()) {
      // beginning room, where player starts at
      auto const center = new_center;

      starting_position.x = center.x;
      starting_position.y = center.y;
    } else {
      // all rooms after the first:
      // connect it to the previous room with a tunnel

      // center coordinates of the previous room
      auto const prev_center = rects[rects.size() - 1].center();

      if (rng.gen_bool()) {
        // first move horizontally, then vertically
        create_h_tunnel(prev_center.x, new_center.x, prev_center.y, tmap);
        create_v_tunnel(prev_center.y, new_center.y, new_center.x, tmap);
      } else {
        // first move vertically, then horizontally
        create_v_tunnel(prev_center.y, new_center.y, prev_center.x, tmap);
        create_h_tunnel(prev_center.x, new_center.x, new_center.y, tmap);
      }
    }

    // add content to the room
    place_objects(new_room, tmap, rng);

    // finally, append the new room to the list
    rects.emplace_back(new_room);
  }
  return Rooms{MOVE(rects), MOVE(starting_position)};
}

TilePosition
place_rooms_and_stairs(TilemapConfig &tconfig, TileMap &tmap,
    stlw::float_generator &rng, entt::DefaultRegistry &registry)
{
  auto const& sc = tconfig.stairconfig;
  auto const stairs_perfloor = sc.stairs_perfloor;
  assert(stairs_perfloor > 0);

  boost::optional<Rooms> rooms = boost::none;
  bool stairs = false;

  while(!rooms && !stairs) {
    std::cerr << "creating rooms ...\n";
    while(!rooms) {
      rooms = create_rooms(tconfig.width, tconfig.length, tmap, rng);
    }
    while(!stairs) {
      std::cerr << "placing stairs ...\n";
      stairs = stairwell_generator::place_stairs(sc, tmap, rng, registry);
    }
  }
  // hack
  auto const add_river = [&](auto const x, auto const y) {
    auto &tile = tmap.data(x, y);
    if (tile.type != TileType::WALL) {
      return;
    }
    tile.type = TileType::RIVER;

    auto &ri = registry.assign<RiverInfo>(tile.eid);
    ri.position = glm::vec3{x, 0, y};
    ri.speed    = 100;
    ri.left     = glm::vec3{x, 0, 0};
    ri.right    = glm::vec3{x + 30, 0, 0};
    ri.top      = glm::vec3{0, 0, 0};
    ri.bottom   = glm::vec3{0, 0, y + 10};
  };
  FOR(i, 8) {
    FOR(j, 8) {
      add_river(i, j);
    }
  }

  // This seems hacky?
  return (*rooms).starting_position;
}

std::pair<TileMap, TilePosition>
make_tilemap(TilemapConfig &tconfig, stlw::float_generator &rng, entt::DefaultRegistry &registry)
{
  // clang-format off
  int const width     = tconfig.width;
  int const length    = tconfig.length;
  int const num_tiles = width * length;
  // clang-format on

  std::vector<Tile> tiles{static_cast<std::size_t>(num_tiles)};
  tiles.reserve(width * length);
  TileMap tmap{MOVE(tiles), width, length, registry};

  std::cerr << "======================================\n";
  auto starting_pos = place_rooms_and_stairs(tconfig, tmap, rng, registry);
  std::cerr << "======================================\n";

  return std::make_pair(MOVE(tmap), MOVE(starting_pos));
}

} // ns boomhs::level_generator
