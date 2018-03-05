#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/tile.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <array>
#include <vector>

namespace boomhs
{
class EntityRegistry;

enum class TileLookupBehavior
{
  ALL_8_DIRECTIONS = 0,
  VERTICAL_HORIZONTAL_ONLY
};

class TileNeighbors
{
  std::vector<TilePosition> neighbors_;
public:
  explicit TileNeighbors(std::vector<TilePosition> &&n)
    : neighbors_(MOVE(n))
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(TileNeighbors);

  auto size() const { return neighbors_.size(); }
  bool empty() const { return neighbors_.empty(); }

  TilePosition const&
  operator[](size_t const i) const
  {
    assert(i < size());
    return neighbors_[i];
  }

  auto const& front() const { return neighbors_.front(); }
};

struct FlowDirection
{
  Tile const& tile;
  glm::vec2 const direction;

  static FlowDirection
  find_flow(Tile const&, std::vector<FlowDirection> const&);
};

struct TileComponent {};

class TileGrid
{
  std::array<size_t, 2> dimensions_;
  EntityRegistry &registry_;

  std::vector<Tile> tiles_ {};
  std::vector<FlowDirection> flowdirs_ {};
  bool destroy_entities_ = true;

public:
  NO_COPY(TileGrid);
  NO_MOVE_ASSIGN(TileGrid);

  TileGrid(size_t const, size_t const, EntityRegistry &);

  ~TileGrid();

  TileGrid(TileGrid &&);

  auto dimensions() const { return dimensions_; }
  bool empty() const { return tiles_.empty(); }
  auto num_tiles() const { return tiles_.size(); }

  Tile& data(size_t const, size_t const);
  Tile const& data(size_t const, size_t const) const;

  Tile& data(TilePosition const& tp) { return data(tp.x, tp.y); }
  Tile const& data(TilePosition const& tp) const { return data(tp.x, tp.y); }

  void assign_bridge(Tile &);
  void assign_river(Tile &, glm::vec2 const&);
  auto const& flows() const { return flowdirs_; }

  bool is_visible(Tile &);
  void set_isvisible(Tile &, bool);

  bool
  is_edge_tile(TilePosition const& tpos) const
  {
    auto const [w, h] = dimensions();
    uint64_t const x = tpos.x, y = tpos.y;
    return ANYOF(x == 0, x == (w - 1), y == 0, y == (h - 1));
  }
  BEGIN_END_FORWARD_FNS(tiles_);
};

} // ns boomhs
