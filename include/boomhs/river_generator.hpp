#pragma once
#include <boomhs/tile.hpp>
#include <stlw/math.hpp>
#include <vector>

namespace stlw
{
class float_generator;
} // ns stlw

namespace boomhs
{
class TileGrid;

struct RiverWiggle
{
  bool is_visible;
  float speed;
  glm::vec2 offset;

  // NOT a TilePosition, because we are tracking it's discrete position for rendering (not it's
  // actual tile position, which is irrelevant for the RiverWiggle
  glm::vec2 position;

  TilePosition
  as_tileposition() const;

  // normalized
  glm::vec2 direction;

  MOVE_ONLY(RiverWiggle);
};

struct RiverInfo
{
  TilePosition origin;
  uint64_t left, top, right, bottom;

  // normalized
  glm::vec2 flow_direction;
  float wiggle_rotation;

  std::vector<RiverWiggle> wiggles = {};
  MOVE_ONLY(RiverInfo);

  template<typename FN, typename ...Args>
  void
  visit_each(FN const& fn, Args &&... args)
  {
    for (auto &w: wiggles) {
      fn(w, std::forward<Args>(args)...);
    }
  }
};

struct RiverGenerator
{
  RiverGenerator() = delete;

  static void
  place_rivers(TileGrid &, stlw::float_generator &, std::vector<RiverInfo> &);
};

} // ns boomhs
