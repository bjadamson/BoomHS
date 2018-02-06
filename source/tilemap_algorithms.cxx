#include <boomhs/tilemap_algorithms.hpp>
#include <boomhs/world_object.hpp>
#include <glm/gtx/string_cast.hpp>

namespace
{

using namespace boomhs;
void
bresenham_3d(int x0, int z0, int x1, int z1, TileMap &tmap)
{
  int const dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int const dz = abs(z1-z0), sz = z0<z1 ? 1 : -1;
  auto const arr = stlw::make_array<int>(dx, 1, dz);

  //int const dm = std::max(dx,dy,dz); /* maximum difference */
  auto const it = std::max_element(arr.cbegin(), arr.cend());
  assert(it);
  int const dm = *it;
  int i = dm;
  x1 = z1 = dm / 2;

  bool found_wall = false;
  auto const set_tile = [&found_wall](auto &tile) {
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
  while(true) {
    auto &tile = tmap.data(x0, z0);
    set_tile(tile);
    if (i-- == 0) break;
    x1 -= dx; if (x1 < 0) { x1 += dm; x0 += sx; }
    z1 -= dz; if (z1 < 0) { z1 += dm; z0 += sz; }
  }
}

} // ns anon

namespace boomhs::detail
{

Edges
calculate_edges(TilePosition const& tpos, int const tmap_width, int const tmap_length,
    int const distance)
{
  auto const left   = std::max(tpos.x - distance, 0);
  auto const right  = std::min(tpos.x + distance, tmap_width);

  auto const top    = std::min(tpos.z + distance, tmap_length);
  auto const bottom = std::max(tpos.z - distance, 0);

  assert(left <= right);
  assert(bottom <= top);

  return Edges{tpos, left, top, right, bottom};
}

} // ns boomhs::detail

namespace boomhs
{

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

bool
any_tilemap_neighbors(TileMap const& tmap, TilePosition const& pos, int32_t const distance,
  bool (*fn)(Tile const&))
{
  auto const [width, length] = tmap.dimensions();
  assert(width > 0);
  assert(length > 0);
  assert(distance > 0);
  assert(length > distance);
  assert(width > distance);
  auto const edge = detail::calculate_edges(pos, width, length, distance);

  bool found_one = false;
  auto const any_neighbors = [&](auto const& neighbor_pos) {
    if (found_one) {
      return;
    }
    if (fn(tmap.data(neighbor_pos))) {
      found_one = true;
    }
  };
  flood_visit_skipping_position(edge, any_neighbors);
  return found_one;
}

void
update_visible_tiles(TileMap &tmap, WorldObject const& player, bool const reveal_tilemap)
{
  // Collect all the visible tiles for the player
  auto const& wp = player.world_position();
  auto const fn = [&](auto const& pos) {
    auto const x = pos.x, z = pos.z;
    if (reveal_tilemap) {
      tmap.data(pos).is_visible = true;
    } else {
      bresenham_3d(wp.x, wp.z, x, z, tmap);
    }
  };
  tmap.visit_each(fn);
}

} // ns boomhs
