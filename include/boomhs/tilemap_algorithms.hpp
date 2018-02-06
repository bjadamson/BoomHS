#pragma once
#include <boomhs/tile.hpp>
#include <boomhs/tilemap.hpp>

#include <array>
#include <iostream>

namespace boomhs
{

struct Edges
{
  TilePosition const position;
  int32_t const left, top, right, bottom;
};

struct Edges;
std::ostream&
operator<<(std::ostream &, Edges const&);

} // ns boomhs

namespace boomhs::detail
{

Edges
calculate_edges(TilePosition const& tpos, int const, int const, int const);

} // ns boomhs::detail

namespace boomhs
{

template<typename FN>
void
flood_visit_skipping_position(Edges const& edge, FN const& fn)
{
  for(auto x = edge.left; x <= edge.right; ++x) {
    for (auto y = edge.bottom; y <= edge.top; ++y) {
      if (edge.position == std::make_pair(x, y)) {
      //if (x == edge.position.x || y == edge.position.y) {
        // skip over the tile (only iterating edges), not original tile
        continue;
      }
      fn(TilePosition{x, y});
    }
  }
}

struct FindNeighborConfig
{
  TileLookupBehavior const behavior;
  uint32_t const distance;
};

template<typename FN>
TileNeighbors
find_neighbors(TileMap const& tmap, TilePosition const& tpos, FN const& fn,
    FindNeighborConfig const& config)
{
  auto const [width, length] = tmap.dimensions();
  auto const distance = config.distance;
  assert(length > 0);
  assert(width > 0);
  assert(distance > 0);

  auto const edges = detail::calculate_edges(tpos, width, length, distance);
  std::vector<TilePosition> neighbors;
  auto const collect_neighbor_positions = [&](auto const& neighbor_pos)
  {
    switch (config.behavior) {
      case TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY:
        {
          if (tpos.x != neighbor_pos.x && tpos.y != neighbor_pos.y) {
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
  flood_visit_skipping_position(edges, collect_neighbor_positions);
  return TileNeighbors{MOVE(neighbors)};
}

inline TileNeighbors
find_neighbors(TileMap const& tmap, TilePosition const& tpos, TileType const type,
    FindNeighborConfig const& config)
{
  auto const fn = [&](auto const& neighbor_tile) { return neighbor_tile.type == type; };
  return find_neighbors(tmap, tpos, fn, config);
}

template<typename FN>
TileNeighbors
find_immediate_neighbors(TileMap const& tmap, TilePosition const& tpos,
    TileLookupBehavior const behavior, FN const& fn)
{
  size_t static constexpr DISTANCE = 1;
  FindNeighborConfig const config{behavior, DISTANCE};
  return find_neighbors(tmap, tpos, fn, config);
}

inline TileNeighbors
find_immediate_neighbors(TileMap const& tmap, TilePosition const& tpos, TileType const type,
    TileLookupBehavior const behavior)
{
  size_t static constexpr DISTANCE = 1;
  FindNeighborConfig const config{behavior, DISTANCE};
  return find_neighbors(tmap, tpos, type, config);
}

bool
any_tilemap_neighbors(TileMap const&, TilePosition const&, int32_t const, bool (*)(Tile const&));

class WorldObject;
void
update_visible_tiles(TileMap &, WorldObject const&, bool const);

} // ns boomhs
