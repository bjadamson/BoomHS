#include <boomhs/level_generator.hpp>
#include <boomhs/tilemap.hpp>
#include <stlw/random.hpp>
#include <stlw/type_macros.hpp>
#include <algorithm>
#include <boost/optional.hpp>
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
    auto const [w, h, l] = tmap.dimensions();
    auto const cast = [](auto const v) { return static_cast<int>(v); };
    return (x1 > 0) && (y1 > 0) && (x2 < cast(w)) && (y2 < cast(l));
  }
};

struct CreateRoomParameters
{
  int const max_width;
  int const max_height;
  std::vector<Rect> const& rooms;
  TileMap &tmap;
  stlw::float_generator &rng;
};

boost::optional<Rect>
try_create_room(CreateRoomParameters &&params)
{
  auto &rng = params.rng;
  // random width and height
  auto const w = rng.gen_int_range(ROOM_MIN_SIZE, ROOM_MAX_SIZE + 1);
  auto const h = rng.gen_int_range(ROOM_MIN_SIZE, ROOM_MAX_SIZE + 1);

  // random position without going out of the boundaries of the map
  auto const x = rng.gen_int_range(0, params.max_width - w);
  auto const y = rng.gen_int_range(0, params.max_height - h);
  Rect const new_room{x, y, w, h};

  // run through the other rooms and see if they intersect with this one
  for(auto const& r : params.rooms) {
    // bail early
    if (!new_room.in_tilemap(params.tmap) || new_room.intersects_with(r)) {
      return {}; // NONE
    }
  }
  for(int x = new_room.x1 + 1; x < new_room.x2; ++x) {
    for (int y = new_room.y1 + 1; y < new_room.y2; ++y) {
      params.tmap.data(x, 0, y).is_wall = false;
    }
  }
  return new_room;
}

Rect
create_room(CreateRoomParameters &&params)
{
  assert(params.max_height - ROOM_MAX_SIZE > 0);
  boost::optional<Rect> room;

  while(!room) {
    room = try_create_room(MOVE(params));
  }
  assert(room);
  return *room;
}

void
create_h_tunnel(int const x1, int const x2, int const y, TileMap &tmap)
{
  int const min = std::min(x1, x2), max = std::max(x1, x2) + 1;

  for (auto x = min; x <= max; ++x) {
    tmap.data(x, 0, y).is_wall = false;
  }
}

void
create_v_tunnel(int const y1, int const y2, int const x, TileMap &tmap)
{
  int const min = std::min(y1, y2), max = std::max(y1, y2) + 1;
  for(auto y = min; y <= max; ++y) {
    tmap.data(x, 0, y).is_wall = false;
  }
}

bool
is_blocked(int const x, int const y, TileMap const& tmap)
{
  if (tmap.data(x, 0, y).is_wall) {
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
  return TilePosition{x, 0, y};
}

void
place_objects(Rect const& room, TileMap const& tmap, stlw::float_generator &rng)
{
  auto const num_monsters = rng.gen_int_range(0, MAX_ROOM_MONSTERS + 1);

  FOR(i, num_monsters) {
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

std::pair<TileMap, TilePosition>
make_tilemap(int const width, int const height, int const length, stlw::float_generator &rng)
{
  int const num_tiles = width * height * length;
  std::vector<Tile> tiles{static_cast<std::size_t>(num_tiles)};
  tiles.reserve(height * width * length);

  auto const cast = [](int const v) { return static_cast<std::size_t>(v); };
  TileMap tmap{MOVE(tiles), cast(width), cast(height), cast(length)};

  std::vector<Rect> rooms;
  TilePosition starting_position;
  FOR(_, MAX_ROOMS) {
    auto const new_room = create_room({width, length, rooms, tmap, rng});
    auto const new_center = new_room.center();

    if (rooms.empty()) {
      // beginning room, where player starts at
      auto const center = new_center;
      starting_position.x = center.x;
      starting_position.z = center.y;
    } else {
      // all rooms after the first:
      // connect it to the previous room with a tunnel

      // center coordinates of the previous room
      auto const prev_center = rooms[rooms.size() - 1].center();

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
    rooms.emplace_back(new_room);
  }
  return std::make_pair(MOVE(tmap), starting_position);
}

} // ns boomhs::level_generator
