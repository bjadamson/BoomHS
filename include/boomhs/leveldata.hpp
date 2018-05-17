#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/fog.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/river_generator.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/tile.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/water.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/lighting.hpp>

#include <stlw/type_macros.hpp>
#include <vector>

namespace boomhs
{

struct LevelGeneratedData
{
  TileGrid               tilegrid;
  TilePosition           startpos;
  std::vector<RiverInfo> rivers;
  Terrain                terrain;
  WaterInfo              water;
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
  TilePosition const  startpos_;

  std::vector<RiverInfo> rivers_;

  Terrain   terrain_;
  WaterInfo water_;

  void set_tile(TilePosition const&, TileType const&);

public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelData);
  LevelData(TileGrid&&, TileSharedInfoTable&&, TilePosition const&, std::vector<RiverInfo>&&,
            Terrain&&, WaterInfo&&, Fog const&, opengl::GlobalLight const&, ObjStore&&,
            WorldObject&&);

  // public fields
  Fog                 fog;
  opengl::GlobalLight global_light;

  ObjStore obj_store;

  // nearby targets user can select
  NearbyTargets nearby_targets;

  WorldObject player;

  // local
  float wind_speed;
  float wave_strength;
  float water_diffuse_offset;

  void set_floor(TilePosition const&);
  void set_river(TilePosition const&);
  void set_wall(TilePosition const&);

  bool is_tile(TilePosition const&, TileType const&) const;
  bool is_wall(TilePosition const&) const;

  auto dimensions() const { return tilegrid_.dimensions(); }

  // TODO: Is this leaky abstraction?
  auto& tilegrid() { return tilegrid_; }

  // Used for rendering
  // TODO: Is this leaky abstraction?
  auto const& tilegrid() const { return tilegrid_; }

  // TODO: maybe accept type parameter, and return TileInfo reference instead of leaking
  // the ttable_ reference?
  auto& tiletable() { return ttable_; }

  auto const& tiletable() const { return ttable_; }

  auto&       rivers() { return rivers_; }
  auto const& rivers() const { return rivers_; }

  auto&       terrain() { return terrain_; }
  auto const& terrain() const { return terrain_; }

  auto&       water() { return water_; }
  auto const& water() const { return water_; }

  template <typename FN>
  void visit_tiles(FN const& fn) const
  {
    visit_each(tilegrid_, fn);
  }
};

} // namespace boomhs
