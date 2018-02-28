#pragma once
#include <boomhs/level_loader.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/river_generator.hpp>
#include <boomhs/tile.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/lighting.hpp>

#include <stlw/type_macros.hpp>
#include <vector>

namespace boomhs
{

struct LevelGeneredData
{
  TileGrid tilegrid;
  TilePosition startpos;
  std::vector<RiverInfo> rivers;
  EntityID torch_eid;
};

class LevelData
{
  // The tilegrid stores stores information about the grid, like it's dimensions, and stores all of
  // the tile's that it owns internally.
  TileGrid tilegrid_;

  // The TileSharedInfoTable "table" stores shared information between the tiles of the same type.
  // For example, the "Material" property for each tile type can be looked up (and manipulated)
  // through ttable_.
  TileSharedInfoTable ttable_;
  TilePosition const startpos_;

  std::vector<RiverInfo> rivers_;
  EntityID const torch_eid_;

  void set_tile(TilePosition const&, TileType const&);
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelData);
  LevelData(TileGrid &&, TileSharedInfoTable &&, TilePosition const&, std::vector<RiverInfo> &&,
      EntityID, opengl::Color const&, opengl::GlobalLight const&, ObjCache &&, Camera &&,
      WorldObject &&
      );

  // public fields
  opengl::Color background;
  opengl::GlobalLight global_light;

  ObjCache obj_cache;

  // nearby targets user can select
  NearbyTargets nearby_targets;

  Camera camera;
  WorldObject player;

  void set_floor(TilePosition const&);
  void set_river(TilePosition const&);
  void set_wall(TilePosition const&);

  bool is_tile(TilePosition const&, TileType const&) const;
  bool is_wall(TilePosition const&) const;

  auto dimensions() const { return tilegrid_.dimensions(); }

  // TODO: Is this leaky abstraction?
  auto&
  tilegrid() { return tilegrid_; }

  // Used for rendering
  // TODO: Is this leaky abstraction?
  auto const&
  tilegrid() const { return tilegrid_; }

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

  auto torch_eid() const { return torch_eid_; }

  template<typename FN>
  void
  visit_tiles(FN const& fn) const { tilegrid_.visit_each(fn); }
};

} // ns boomhs
