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

template<typename FN, typename ...Args>
void
bresenham_3d(int const wpx, int const wpz, int x1, int z1, FN const& fn, Args &&... args)
{
  int const dx = std::abs(x1 - wpx);
  int const sx = wpx < x1 ? 1 : -1;

  int const dz = std::abs(z1 - wpz);
  int const sz = wpz < z1 ? 1 : -1;
  auto const arr = stlw::make_array<int>(dx, 1, dz);

  auto const it = std::max_element(arr.cbegin(), arr.cend());
  assert(it);
  int const dm = *it;

  int i = dm;
  assert(i >= 0);
  x1 = z1 = dm / 2;

  int x0 = wpx, z0 = wpz;
  while(true) {
    fn(x0, z0, std::forward<Args>(args)...);
    if (i-- == 0) break;
    x1 -= dx; if (x1 < 0) { x1 += dm; x0 += sx; }
    z1 -= dz; if (z1 < 0) { z1 += dm; z0 += sz; }
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
      tile.is_visible = false;
    }
    else if (!is_wall) {
      tile.is_visible = true;
    } else if (is_wall) {
      found_wall = true;
      tile.is_visible = true;
    } else {
      tile.is_visible = false;
    }
  };

  // Collect all the visible tiles for the player
  auto const fn = [&](TilePosition const& pos) {
    if (reveal_tilegrid) {
      tilegrid.data(pos).is_visible = true;
    } else {
      auto const& wp = player.world_position();

      bool found_wall = false;
      bresenham_3d(wp.x, wp.z, pos.x, pos.y, set_tile, found_wall);
    }
  };
  tilegrid.visit_each(fn);
}

void
update_visible_riverwiggles(LevelData &ldata, WorldObject const& player, bool const reveal_tilegrid)
{
  auto &tilegrid = ldata.tilegrid();
  auto const set_tile = [&tilegrid](int const x0, int const z0, auto &wiggle,
      bool &found_river, bool &found_wall)
  {
    if (found_river || found_wall) {
      return;
    }
    auto &tile = tilegrid.data(x0, z0);
    if (TileType::RIVER == tile.type) {
      // first notable tile we found is a river, mark this wiggle visible
      wiggle.is_visible = true;
      found_river = true;
    }
    else if (TileType::WALL == tile.type) {
      wiggle.is_visible = false;
      found_wall = true;
    }
  };

  auto const fn = [&](RiverWiggle &wiggle) {
    if (reveal_tilegrid) {
      wiggle.is_visible = true;
    } else {
      auto const& wp = player.world_position();
      auto const pos = wiggle.as_tileposition();

      bool found_river = false, found_wall = false;
      bresenham_3d(wp.x, wp.z, pos.x, pos.y, set_tile, wiggle, found_river, found_wall);
    }
  };
  auto &rinfos = ldata.rivers();
  for (auto &rinfo : rinfos) {
    rinfo.visit_each(fn);
  }
}

} // ns boomhs
