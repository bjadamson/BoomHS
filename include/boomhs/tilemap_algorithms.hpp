#pragma once
#include <boomhs/tile.hpp>
#include <boomhs/tilemap.hpp>

#include <array>

namespace boomhs::detail
{

struct Edges
{
  TilePosition position;
  int32_t const left, top, right, bottom;

  template<typename FN>
  void
  visit_each(FN const& fn) const
  {
    for(auto z = left; z <= right; ++z) {
      for (auto x = bottom; x <= top; ++x) {
        if (position == std::make_pair(z, x)) {
          // skip over the tile (only iterating edges), not original tile
          continue;
        }
        fn(TilePosition{x, z});
      }
    }
  }
};

std::ostream&
operator<<(std::ostream &, Edges const&);

Edges
calculate_edges(TilePosition const& tpos, int const, int const, int const);

} // ns boomhs::detail

namespace boomhs
{

template<typename FN>
TileNeighbors
find_neighbors(TileMap const& tmap, TilePosition const& tpos, TileLookupBehavior const behavior,
    FN const& fn)
{
  static auto constexpr distance = 1;
  auto const [width, length] = tmap.dimensions();
  assert(length > 0);
  assert(width > 0);
  assert(distance > 0);

  auto const edges = detail::calculate_edges(tpos, width, length, distance);
  std::vector<TilePosition> neighbors;
  auto count = 0ul;
  auto const count_neighbors = [&](auto const& neighbor_pos)
  {
    switch (behavior) {
      case TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY:
        {
          if (tpos.x != neighbor_pos.x && tpos.z != neighbor_pos.z) {
            // skip neighbors not on same horiz/vert planes
            return;
          }
        }
        // Explicitely use fallthrough for code reuse
      case TileLookupBehavior::ALL_8_DIRECTIONS:
        {
          auto const& neighbor_tile = tmap.data(neighbor_pos);
          if (fn(neighbor_tile)) {
            neighbors.emplace_back(neighbor_pos);
          }
        }
        break;
      default:
        std::abort();
        break;
    }
  };
  edges.visit_each(count_neighbors);
  return TileNeighbors{MOVE(neighbors)};
}

inline TileNeighbors
find_neighbors(TileMap const& tmap, TilePosition const& tpos, TileType const type,
    TileLookupBehavior const behavior)
{
  auto const fn = [](auto const& neighbor_tile) { return neighbor_tile.type == TileType::WALL; };
  return find_neighbors(tmap, tpos, behavior, fn);
}

class TileMap;
size_t
adjacent_neighbor_tilecount(TileType const, TilePosition const&, TileMap const&,
    TileLookupBehavior const);

bool
any_tilemap_neighbors(TileMap const&, TilePosition const&, int32_t const, bool (*)(Tile const&));

class WorldObject;
void
update_visible_tiles(TileMap &, WorldObject const&, bool const);

} // ns boomhs
