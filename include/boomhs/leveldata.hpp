#pragma once
#include <boomhs/river_generator.hpp>
#include <boomhs/tile.hpp>
#include <boomhs/tiledata.hpp>
#include <stlw/type_macros.hpp>
#include <vector>

namespace boomhs
{

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
