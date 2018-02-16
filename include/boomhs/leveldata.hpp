#pragma once
#include <boomhs/level_loader.hpp>
#include <boomhs/river_generator.hpp>
#include <boomhs/tile.hpp>
#include <boomhs/tiledata.hpp>

#include <opengl/lighting.hpp>

#include <stlw/type_macros.hpp>
#include <vector>

namespace boomhs
{

class LevelData
{
  // The tilegrid stores stores information about the grid, like it's dimensions, and stores all of
  // the tile's that it owns internally.
  TileData tilegrid_;

  // The TileSharedInfoTable "table" stores shared information between the tiles of the same type.
  // For example, the "Material" property for each tile type can be looked up (and manipulated)
  // through ttable_.
  TileSharedInfoTable ttable_;
  TilePosition const startpos_;

  std::vector<RiverInfo> rivers_;

  void set_tile(TilePosition const&, TileType const&);
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelData);
  LevelData(TileData &&, TileSharedInfoTable &&, TilePosition const&, std::vector<RiverInfo> &&);

  void set_floor(TilePosition const&);
  void set_river(TilePosition const&);
  void set_wall(TilePosition const&);

  bool is_tile(TilePosition const&, TileType const&) const;
  bool is_wall(TilePosition const&) const;

  auto dimensions() const { return tilegrid_.dimensions(); }

  // TODO: Is this leaky abstraction?
  auto&
  tiledata() { return tilegrid_; }

  // Used for rendering
  // TODO: Is this leaky abstraction?
  auto const&
  tiledata() const { return tilegrid_; }

  // TODO: maybe accept type parameter, and return TileInfo reference instead of leaking
  // the ttable_ reference?
  auto&
  tiletable() { return ttable_; }

  auto const&
  tiletable() const { return ttable_; }

  auto&
  rivers() { return rivers_; }

  auto const&
  rivers() const { return rivers_; }

  template<typename FN>
  void
  visit_tiles(FN const& fn) const { tilegrid_.visit_each(fn); }
};

} // ns boomhs
