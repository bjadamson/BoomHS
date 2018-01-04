#include <boomhs/tilemap.hpp>
#include <boomhs/player.hpp>

namespace
{

using namespace boomhs;
void
bresenham_3d(int x0, int y0, int z0, int x1, int y1, int z1, TileMap &tmap)
{
  int const dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int const dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int const dz = abs(z1-z0), sz = z0<z1 ? 1 : -1;
  auto const arr = stlw::make_array<int>(dx, dy, dz);

  //int const dm = std::max(dx,dy,dz); /* maximum difference */
  auto const it = std::max_element(arr.cbegin(), arr.cend());
  assert(it);
  int const dm = *it;
  int i = dm;
  x1 = y1 = z1 = dm/2; /* error offset */

  bool found_wall = false;
  auto const set_tile = [&found_wall](auto &tile) {
    if (found_wall) {
      // Can't see tile's behind a wall.
      tile.is_visible = false;
    }
    else if (!tile.is_wall) {
      tile.is_visible = true;
    } else if (tile.is_wall) {
      found_wall = true;
      tile.is_visible = true;
    } else {
      tile.is_visible = false;
    }
  };

   for(;;) {  /* loop */
     auto &tile = tmap.data(x0, y0, z0);
      set_tile(tile);
      if (i-- == 0) break;
      x1 -= dx; if (x1 < 0) { x1 += dm; x0 += sx; }
      y1 -= dy; if (y1 < 0) { y1 += dm; y0 += sy; }
      z1 -= dz; if (z1 < 0) { z1 += dm; z0 += sz; }
   }
}

} // ns anon

namespace boomhs
{

void
update_visible_tiles(TileMap &tmap, Player const& player, bool const reveal_tilemap)
{
  auto const& wp = player.world_position();

  // Collect all the visible tiles for the player
  auto const [w, h, l] = tmap.dimensions();

  std::vector<TilePosition> visited;
  auto const update_tile = [&tmap, &visited](TilePosition const& pos) {
    bool found_wall = false;
      auto &tile = tmap.data(pos.x, pos.y, pos.z);
      if (!found_wall && !tile.is_wall) {
        // This is probably not always necessary. Consider starting with all tiles visible?
        tile.is_visible = true;
      }
      else if(!found_wall && tile.is_wall) {
        tile.is_visible = true;
        found_wall = true;
      } else {
        tile.is_visible = false;
      }
    };

  std::vector<TilePosition> positions;
  FOR(x, w) {
    FOR(y, h) {
      FOR (z, l) {
        if (reveal_tilemap) {
          tmap.data(x, y, z).is_visible = true;
        } else {
          bresenham_3d(wp.x, wp.y, wp.z, x, y, z, tmap);
        }
      }
    }
  }
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
  for (auto const& ppos : visited) {
    update_tile(ppos);
  }
}

} // ns boomhs
