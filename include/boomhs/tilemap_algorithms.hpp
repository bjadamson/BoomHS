#pragma once
#include <boomhs/tile.hpp>
#include <boomhs/tilemap.hpp>

#include <array>

namespace boomhs::detail
{

struct Edges
{
  int32_t const left, top, right, bottom;

  template<typename FN>
  void
  visit_each(FN const& fn) const
  {
    for(auto x = left; x <= right; ++x) {
      for (auto y = bottom; y <= top; ++y) {
        fn(TilePosition{x, y});
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
  std::array<TilePosition, 8> neighbors;
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
            neighbors[count++] = neighbor_pos;
          }
        }
        break;
      default:
        std::abort();
        break;
    }
  };
  edges.visit_each(count_neighbors);
  return TileNeighbors{count, MOVE(neighbors)};
}

class TileMap;
size_t
adjacent_neighbor_tilecount(TileType const, TilePosition const&, TileMap const&,
    TileLookupBehavior const);

bool
any_tilemap_neighbors(TileMap const&, TilePosition const&, int32_t const, bool (*)(Tile const&));

TileNeighbors
find_neighbors(TileMap const&, TilePosition const&, TileType const, TileLookupBehavior const);

class WorldObject;
void
update_visible_tiles(TileMap &, WorldObject const&, bool const);

} // ns boomhs
