#pragma once
#include <boomhs/tile.hpp>
#include <boomhs/tiledata.hpp>

#include <array>
#include <iostream>

namespace stlw
{
class float_generator;
} // ns stlw

namespace boomhs
{

struct Edges
{
  TilePosition const position;
  size_t const left, top, right, bottom;
};

Edges
calculate_edges(TilePosition const& tpos, size_t const, size_t const, size_t const);

struct Edges;
std::ostream&
operator<<(std::ostream &, Edges const&);

template<typename FN>
void
flood_visit_skipping_position(Edges const& edge, FN const& fn)
{
  for(auto x = edge.left; x <= edge.right; ++x) {
    for (auto y = edge.bottom; y <= edge.top; ++y) {
      if (edge.position == std::make_pair(x, y)) {
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
find_neighbors(TileData const& tdata, TilePosition const& tpos, FN const& fn,
    FindNeighborConfig const& config)
{
  auto const [width, height] = tdata.dimensions();
  auto const distance = config.distance;
  assert(width > 0);
  assert(height > 0);
  assert(distance > 0);

  auto const edges = calculate_edges(tpos, width, height, distance);
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
          auto const& neighbor_tile = tdata.data(neighbor_pos);
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
find_neighbors(TileData const& tdata, TilePosition const& tpos, TileType const type,
    FindNeighborConfig const& config)
{
  auto const fn = [&](auto const& neighbor_tile) { return neighbor_tile.type == type; };
  return find_neighbors(tdata, tpos, fn, config);
}

template<typename FN>
TileNeighbors
find_immediate_neighbors(TileData const& tdata, TilePosition const& tpos,
    TileLookupBehavior const behavior, FN const& fn)
{
  size_t static constexpr DISTANCE = 1;
  FindNeighborConfig const config{behavior, DISTANCE};
  return find_neighbors(tdata, tpos, fn, config);
}

inline TileNeighbors
find_immediate_neighbors(TileData const& tdata, TilePosition const& tpos, TileType const type,
    TileLookupBehavior const behavior)
{
  size_t static constexpr DISTANCE = 1;
  FindNeighborConfig const config{behavior, DISTANCE};
  return find_neighbors(tdata, tpos, type, config);
}

class MapEdge
{
public:
  enum Side
  {
    LEFT = 0, TOP, RIGHT, BOTTOM
  };

private:
  Side side_;
public:
  explicit MapEdge(Side const);
  bool is_xedge() const;
  bool is_yedge() const;

  Side side() const { return side_; }

  static MapEdge
  random_edge(stlw::float_generator &);
};

std::pair<TilePosition, MapEdge>
random_tileposition_onedgeofmap(TileData const&, stlw::float_generator &);

bool
any_tiledata_neighbors(TileData const&, TilePosition const&, size_t const, bool (*)(Tile const&));

class WorldObject;
void
update_visible_tiles(TileData &, WorldObject const&, bool const);

} // ns boomhs
