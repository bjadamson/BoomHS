#pragma once
#include <boomhs/components.hpp>
#include <boomhs/tile.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>
#include <array>
#include <vector>

namespace boomhs
{

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

class TileMap
{
  std::array<int32_t, 2> dimensions_;
  entt::DefaultRegistry &registry_;

  std::vector<Tile> tiles_;

  bool destroy_entities_ = true;

public:
  NO_COPY(TileMap);
  NO_MOVE_ASSIGN(TileMap);

  TileMap(std::vector<Tile> &&, int32_t const, int32_t const, entt::DefaultRegistry &);

  ~TileMap();

  TileMap(TileMap &&);

  auto dimensions() const { return dimensions_; }
  auto num_tiles() const { return tiles_.size(); }

  Tile&
  data(size_t const, size_t const);

  Tile const&
  data(size_t const, size_t const) const;

  Tile&
  data(TilePosition const& tp) { return data(tp.x, tp.y); }

  Tile const&
  data(TilePosition const& tp) const { return data(tp.x, tp.y); }

  template<typename FN>
  void
  visit_each(FN const& fn) const
  {
    auto const [w, l] = dimensions();
    FORI(x, w) {
      FORI(z, l) {
        fn(TilePosition{x, z});
      }
    }
  }

  template<typename FN>
  void
  visit_neighbors(TilePosition const& pos, FN const& fn, TileLookupBehavior const behavior) const
  {
    auto const [w, l] = dimensions();
    assert(w == l); // TODO: test if this works if this assumption not true

    // clang-format off
    bool const edgeof_left  = pos.x == 0;
    bool const edgeof_right = pos.x == static_cast<int>(l - 1);

    bool const edgeof_below = pos.y == 0;
    bool const edgeof_above = pos.y == static_cast<int>(w - 1);

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
