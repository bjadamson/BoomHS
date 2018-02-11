#pragma once
#include <boomhs/tile.hpp>
#include <boomhs/tiledata.hpp>
#include <stlw/type_macros.hpp>

#include <glm/glm.hpp>
#include <vector>

namespace boomhs
{

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

  // stateful information used by rendering routine. Each frame the renderer compares this value
  // against the current ticks(), if the amount of time elapsed is sufficient the renderer.
  uint64_t river_timer = 0ul;

  // TODO: try re-enabling once we switch from boost::optional to std::optional (c++17)
  //MOVE_CONSTRUCTIBLE_ONLY(RiverInfo);
};

class LevelData
{
  TileData tilegrid_;
  TilePosition const startpos_;

  std::vector<RiverInfo> rivers_;

  void set_tile(TilePosition const&, TileType const&);
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelData);
  LevelData(TileData &&, TilePosition const&, std::vector<RiverInfo> &&);

  void set_floor(TilePosition const&);
  void set_river(TilePosition const&);
  void set_wall(TilePosition const&);

  bool is_tile(TilePosition const&, TileType const&) const;
  bool is_wall(TilePosition const&) const;

  auto dimensions() const { return tilegrid_.dimensions(); }

  auto&
  tiledata_mutref() { return tilegrid_; }

  // Used for rendering
  auto const&
  tiledata() const { return tilegrid_; }

  auto&
  rivers_mutref() { return rivers_; }

  auto const&
  rivers() const { return rivers_; }

  template<typename FN>
  void
  visit_tiles(FN const& fn) const { tilegrid_.visit_each(fn); }
};

} // ns boomhs
