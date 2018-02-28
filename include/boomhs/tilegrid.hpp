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

  template<typename FN, typename ...Args>
  void
  visit_each(FN const& fn, Args &&... args) const
  {
    auto const [w, h] = dimensions();
    FOR(x, w) {
      FOR(y, h) {
        fn(TilePosition{x, y}, std::forward<Args>(args)...);
      }
    }
  }

  template<typename FN, typename ...Args>
  void
  visit_edges(FN const& fn, Args &&... args) const
  {
    auto const [w, h] = dimensions();
    auto const visit_fn = [&fn, this](TilePosition const& tpos) {
      if (is_edge_tile(tpos)) {
        fn(tpos);
      }
    };
    visit_each(visit_fn, FORWARD(args)...);
  }

  bool
  is_edge_tile(TilePosition const& tpos) const
  {
    auto const [w, h] = dimensions();
    uint64_t const x = tpos.x, y = tpos.y;
    return ANYOF(x == 0, x == (w - 1), y == 0, y == (h - 1));
  }

  template<typename FN>
  void
  visit_neighbors(TilePosition const& pos, FN const& fn, TileLookupBehavior const behavior) const
  {
    auto const [w, y] = dimensions();
    assert(w == y); // TODO: test if this works if this assumption not true
    assert(pos.x < w);
    assert(pos.y < y);

    // clang-format off
    bool const edgeof_left  = pos.x == 0;
    bool const edgeof_right = pos.x == (y - 1);

    bool const edgeof_below = pos.y == 0;
    bool const edgeof_above = pos.y == (w - 1);

    auto const leftbelow  = [&]() { fn(pos.x + -1, pos.y + -1); };
    auto const left       = [&]() { fn(pos.x + 0,  pos.y + -1); };
    auto const leftabove  = [&]() { fn(pos.x + 1,  pos.y + -1); };
    auto const above      = [&]() { fn(pos.x + 1,  pos.y + 0); };
    auto const rightabove = [&]() { fn(pos.x + 1,  pos.y + 1); };
    auto const right      = [&]() { fn(pos.x + 0,  pos.y + 1); };
    auto const rightbelow = [&]() { fn(pos.x + -1, pos.y + 1); };
    auto const below      = [&]() { fn(pos.x + -1, pos.y + 0); };
    // clang-format on

    auto const all8_behavior = [&]()
    {
      if (!edgeof_left && !edgeof_below) {
        leftbelow();
      }
      if (!edgeof_left) {
        left();
      }
      if (!edgeof_left && !edgeof_above) {
        leftabove();
      }
      if (!edgeof_above) {
        above();
      }
      if (!edgeof_right && !edgeof_above) {
        rightabove();
      }
      if (!edgeof_right) {
        right();
      }
      if (!edgeof_right && !edgeof_below) {
        rightbelow();
      }
      if (!edgeof_below) {
        below();
      }
    };
    auto const vh_behavior = [&]() {
      if (!edgeof_left) {
        left();
      }
      if (!edgeof_above) {
        above();
      }
      if (!edgeof_right) {
        right();
      }
      if (!edgeof_below) {
        below();
      }
    };
    switch(behavior) {
      case TileLookupBehavior::ALL_8_DIRECTIONS:
        all8_behavior();
        break;
      case TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY:
        vh_behavior();
        break;
      default:
        std::exit(1);
    }
  }
  BEGIN_END_FORWARD_FNS(tiles_);
};

} // ns boomhs
