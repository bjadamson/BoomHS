#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/fog.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/material.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/water.hpp>
#include <boomhs/wind.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/lighting.hpp>

#include <common/type_macros.hpp>
#include <vector>

namespace boomhs
{

struct LevelGeneratedData
{
  TerrainGrid terrain;
};

class LevelData
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelData);
  LevelData(TerrainGrid&&, Fog&&, GlobalLight const&, MaterialTable&&, ObjStore&&);

  // public fields
  Skybox      skybox;
  Fog         fog;
  Wind        wind;
  TerrainGrid terrain;

  GlobalLight   global_light;
  MaterialTable material_table;

  ObjStore obj_store;

  // nearby targets user can select
  NearbyTargets nearby_targets;

  // local
  float time_offset;

  auto dimensions() const { return terrain.max_worldpositions(); }
};

} // namespace boomhs
