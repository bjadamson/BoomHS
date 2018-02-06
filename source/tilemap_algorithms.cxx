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

Edges
calculate_edges(TilePosition const& tpos, int const tmap_width, int const tmap_length,
    int const distance)
{
  auto const left   = std::max(tpos.z - distance, 0);
  auto const right  = std::min(tpos.z + distance, tmap_width);

  auto const top    = std::min(tpos.x + distance, tmap_length);
  auto const bottom = std::max(tpos.x - distance, 0);

  assert(left <= right);
  assert(bottom <= top);

  return Edges{tpos, left, top, right, bottom};
}

} // ns boomhs::detail

namespace boomhs
{

size_t
adjacent_neighbor_tilecount(TileType const type, TilePosition const& pos,
    TileMap const& tmap, TileLookupBehavior const behavior)
{
  size_t neighbor_count = 0u;
  auto const check_neighbor = [&](int const x_offset, int const z_offset) {
    auto const tile = TilePosition{pos.x + x_offset, pos.z + z_offset};
    if (tmap.data(tile).type == type) {
      ++neighbor_count;
    }
  };

  tmap.visit_neighbors(pos, check_neighbor, behavior);
  return neighbor_count;
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
  auto const edges = detail::calculate_edges(pos, width, length, distance);
  std::cerr << "tilepos: '" << pos << "' distance: '" << distance << "' edge: '" << edges << "'\n";

  bool found_one = false;
  auto const any_neighbors = [&](auto const& neighbor_pos) {
    if (found_one) {
      return;
    }
    if (fn(tmap.data(neighbor_pos))) {
      found_one = true;
    }
  };
  edges.visit_each(any_neighbors);
  return found_one;
}

void
update_visible_tiles(TileMap &tmap, WorldObject const& player, bool const reveal_tilemap)
{
  // Collect all the visible tiles for the player
  auto const [w, _] = tmap.dimensions();
  auto const update_tile = [&](TilePosition const& pos) {
    bool found_wall = false;
    auto &tile = tmap.data(pos);
    bool const is_wall = tile.type == TileType::WALL;
    if (!found_wall && !is_wall) {
      // This is probably not always necessary. Consider starting with all tiles visible?
      tile.is_visible = true;
    }
    else if(!found_wall && is_wall) {
      tile.is_visible = true;
      found_wall = true;
    } else {
      tile.is_visible = false;
    }
  };

  // TODO: wtf is going on here, why again am I looping twice? unclear, document or something.
  auto const& wp = player.world_position();
  std::vector<TilePosition> positions;
  auto const fn = [&](auto const& pos) {
    auto const x = pos.x, z = pos.z;
    if (reveal_tilemap) {
      tmap.data(pos).is_visible = true;
    } else {
      bresenham_3d(wp.x, wp.z, x, z, tmap);
    }
  };
  tmap.visit_each(fn);

  std::vector<TilePosition> visited;
  for (auto const& pos : positions) {
    auto const cmp = [&pos](auto const& pcached) {
      return pcached == pos;
    };
    bool const seen_already = std::find_if(visited.cbegin(), visited.cend(), cmp) != visited.cend();
    if (seen_already) {
      // This tile has already been visited, skip
      continue;
    }
    visited.emplace_back(pos);
  }
  for (auto const& tpos : visited) {
    update_tile(tpos);
  }
}

} // ns boomhs
