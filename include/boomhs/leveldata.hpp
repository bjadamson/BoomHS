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

  glm::vec3 position;

  MOVE_CONSTRUCTIBLE_ONLY(RiverWiggle);
};

struct RiverInfo
{
  glm::vec3 left, right, top, bottom;

  std::vector<RiverWiggle> wiggles;

  MOVE_CONSTRUCTIBLE_ONLY(RiverInfo);
};

class LevelData
{
  TileData tilegrid_;
  TilePosition const startpos_;

  std::vector<RiverInfo> rivers_;

  void set_tile(TilePosition const&, TileType const&);
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelData);
  LevelData(TileData &&, TilePosition const&);

  ~LevelData()
  {
    if (!rivers_.empty() || !tilegrid_.empty())
    {
      std::abort();
    }
  }

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
