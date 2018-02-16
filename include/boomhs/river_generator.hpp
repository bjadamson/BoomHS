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
  float speed;
  glm::vec2 offset;

  // NOT a TilePosition, because we are tracking it's discrete position for rendering (not it's
  // actual tile position, which is irrelevant for the RiverWiggle
  glm::vec2 position;

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
};

} // ns boomhs

namespace boomhs::river_generator
{

void
place_rivers(TileGrid &, stlw::float_generator &, std::vector<RiverInfo> &);

} // ns boomhs::river_generator
