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
class TileData;

struct RiverWiggle
{
  float speed;
  float z_jiggle;

  // NOT a TilePosition, because we are tracking it's discrete position for rendering (not it's
  // actual tile position, which is irrelevant for the RiverWiggle
  glm::vec2 position;

  // normalized
  glm::vec2 direction;

  // TODO: try re-enabling once we switch from boost::optional to std::optional (c++17)
  //MOVE_CONSTRUCTIBLE_ONLY(RiverWiggle);
};

struct RiverInfo
{
  TilePosition origin;
  uint64_t left, top, right, bottom;

  // normalized
  glm::vec2 flow_direction;
  float wiggle_rotation;

  std::vector<RiverWiggle> wiggles = {};
  // TODO: try re-enabling once we switch from boost::optional to std::optional (c++17)
  //MOVE_CONSTRUCTIBLE_ONLY(RiverInfo);
};

} // ns boomhs

namespace boomhs::river_generator
{

void
place_rivers(TileData &, stlw::float_generator &, std::vector<RiverInfo> &);

} // ns boomhs::river_generator
