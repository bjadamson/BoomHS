#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/fog.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/water.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/lighting.hpp>

#include <stlw/type_macros.hpp>
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
  LevelData(TerrainGrid&&, Fog const&, opengl::GlobalLight const&, ObjStore&&, WorldObject&&);

  // public fields
  Skybox      skybox;
  Fog         fog;
  TerrainGrid terrain;

  opengl::GlobalLight global_light;

  ObjStore obj_store;

  // nearby targets user can select
  NearbyTargets nearby_targets;

  WorldObject player;

  // local
  float time_offset;

  auto dimensions() const { return terrain.max_worldpositions(); }
};

} // namespace boomhs
