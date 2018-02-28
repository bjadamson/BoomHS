#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/river_generator.hpp>
#include <boomhs/world_object.hpp>

#include <stlw/debug.hpp>
#include <stlw/math.hpp>
#include <stlw/random.hpp>
#include <algorithm>

using namespace boomhs;
static auto constexpr SIDES = stlw::make_array<MapEdge::Side>(
    MapEdge::Side::LEFT,
    MapEdge::Side::TOP,
    MapEdge::Side::RIGHT,
    MapEdge::Side::BOTTOM);

namespace
{

// Adapted from from:
// http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html
template<typename FN, typename ...Args>
void
bresenham_3d(int const x0, int const y0, int const x1, int const y1, FN const& fn, Args &&... args)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int x = x0;
  int y = y0;
  int n = 1 + dx + dy;
  int const x_inc = (x1 > x0) ? 1 : -1;
  int const y_inc = (y1 > y0) ? 1 : -1;
  int error = dx - dy;
  dx *= 2;
  dy *= 2;

  for (; n > 0; --n) {
    fn(x, y, std::forward<Args>(args)...);
    if (error > 0) {
      x += x_inc;
      error -= dy;
    }
    else {
      y += y_inc;
      error += dx;
    }
  }
}

} // ns anon

namespace boomhs
{

Edges
calculate_edges(TilePosition const& tpos, uint64_t const tgrid_width, uint64_t const tgrid_height,
    uint64_t const distance_v, uint64_t const distance_h)
{
  assert(tgrid_width > 0ul);
  assert(tgrid_height > 0ul);

  auto const left   = tpos.x > distance_v ? std::max(tpos.x - distance_v, 0ul) : 0ul;
  auto const right  = std::min(tpos.x + distance_v, tgrid_width - 1);

  auto const top    = std::min(tpos.y + distance_h, tgrid_height - 1);
  auto const bottom = tpos.y > distance_h ? std::max(tpos.y - distance_h, 0ul) : 0ul;

  //std::cerr << "left: '" << left << "' right: '" << right << "'\n";
  assert(left <= right);
  assert(bottom <= top);

  return Edges{tpos, left, top, right, bottom};
}

std::ostream&
operator<<(std::ostream& stream, Edges const& e)
{
  stream << "{";
  stream << "left: "   << e.left;
  stream << ", ";
  stream << "top: "    << e.top;
  stream << ", ";
  stream << "right: "  << e.right;
  stream << ", ";
  stream << "bottom: " << e.bottom;
  stream << "}";
  return stream;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// MapEdge
MapEdge::MapEdge(Side const side)
  : side_(side)
{
}

bool
MapEdge::is_xedge() const
{
  return ANYOF(side_ == LEFT, side_ == RIGHT);
}

bool
MapEdge::is_yedge() const
{
  return ANYOF(side_ == TOP, side_ == BOTTOM);
}

MapEdge
MapEdge::random_edge(stlw::float_generator &rng)
{
  assert(SIDES.size() > 0);

  auto const rand = rng.gen_uint64_range(0, SIDES.size() - 1);
  assert(SIDES.size() > rand);
  auto const side = static_cast<MapEdge::Side>(rand);
  return MapEdge{side};
}

std::pair<TilePosition, MapEdge>
random_tileposition_onedgeofmap(TileGrid const& tilegrid, stlw::float_generator &rng)
{
  auto const edge = MapEdge::random_edge(rng);
  auto const xedge = edge.is_xedge();
  auto const yedge = edge.is_yedge();
  stlw::assert_exactly_only_one_true(xedge, yedge);

  auto const make_pos = [&edge](uint64_t const x, uint64_t const y) {
    TilePosition tpos{x, y};
    return std::make_pair(tpos, edge);
  };
  auto const [tdwidth, tdheight] = tilegrid.dimensions();
  if (xedge) {
    return make_pos(0, rng.gen_uint64_range(0, tdwidth - 1));
  } else {
    assert(yedge);
    return make_pos(rng.gen_uint64_range(0, tdheight - 1), 0);
  }
  std::abort();
  return make_pos(0, 0);
}

bool
any_tilegrid_neighbors(TileGrid const& tilegrid, TilePosition const& pos, uint64_t const distance,
  bool (*fn)(Tile const&))
{
  auto const [width, length] = tilegrid.dimensions();
  assert(width > 0);
  assert(length > 0);
  assert(distance > 0);
  assert(length > distance);
  assert(width > distance);
  auto const edge = calculate_edges(pos, width, length, distance, distance);

  bool found_one = false;
  auto const any_neighbors = [&](auto const& neighbor_pos) {
    if (found_one) {
      return;
    }
    if (fn(tilegrid.data(neighbor_pos))) {
      found_one = true;
    }
  };
  flood_visit_skipping_position(edge, any_neighbors);
  return found_one;
}

void
update_visible_tiles(TileGrid &tilegrid, WorldObject const& player, bool const reveal_tilegrid)
{
  auto const set_tile = [&tilegrid](int const x0, int const z0, bool &found_wall)
  {
    auto &tile = tilegrid.data(x0, z0);
    bool const is_wall = tile.type == TileType::WALL;
    if (found_wall) {
      // Can't see tile's behind a wall.
      tilegrid.set_isvisible(tile, false);
    }
    else if (!is_wall) {
      tilegrid.set_isvisible(tile, true);
    } else if (is_wall) {
      found_wall = true;
      tilegrid.set_isvisible(tile, true);
    } else {
      tilegrid.set_isvisible(tile, false);
    }
  };

  // Collect all the visible tiles for the player
  auto const fn = [&](TilePosition const& pos) {
    if (reveal_tilegrid) {
      auto &tile = tilegrid.data(pos);
      tilegrid.set_isvisible(tile, true);
    } else {
      auto const& wp = player.world_position();

      bool found_wall = false;
      bresenham_3d(wp.x, wp.z, pos.x, pos.y, set_tile, found_wall);
    }
  };
  visit_each(tilegrid, fn);
}

void
update_visible_riverwiggles(LevelData &ldata, WorldObject const& player, bool const reveal_tilegrid)
{
  auto const set_tile = [&ldata](int const x0, int const z0, auto const& wp, auto &wiggle,
      bool &found_losblock)
  {
    if (found_losblock) {
      return;
    }
    auto &tilegrid = ldata.tilegrid();
    auto &tile = tilegrid.data(x0, z0);
    if (TileType::RIVER == tile.type) {
      // tile is a river, mark visible
      wiggle.is_visible = true;
    }
    else if (TileType::WALL == tile.type) {
      wiggle.is_visible = false;
      found_losblock = true;
    }
  };

  auto const fn = [&](RiverWiggle &wiggle) {
    if (reveal_tilegrid) {
      wiggle.is_visible = true;
    } else {
      auto const& wp = player.world_position();
      auto const pos = wiggle.as_tileposition();

      bool found_losblock = false;
      bresenham_3d(wp.x, wp.z, pos.x, pos.y, set_tile, wp, wiggle, found_losblock);
    }
  };
  auto &rinfos = ldata.rivers();
  for (auto &rinfo : rinfos) {
    rinfo.visit_each(fn);
  }
}

} // ns boomhs
