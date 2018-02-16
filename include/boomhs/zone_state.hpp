#pragma once
#include <boomhs/camera.hpp>
#include <boomhs/world_object.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/level_loader.hpp>

#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/lighting.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>
#include <optional>
#include <vector>

namespace boomhs
{

struct GpuState
{
  // These slots get a value when memory is loaded, set to none when memory is not.
  std::optional<opengl::EntityDrawHandles> entities;
  std::optional<opengl::TileDrawHandles> tiles;

  MOVE_CONSTRUCTIBLE_ONLY(GpuState);
};

struct GfxState
{
  GpuState gpu_state = {};
  opengl::ShaderPrograms sps;
  opengl::TextureTable texture_table;

  explicit GfxState(opengl::ShaderPrograms &&sp, opengl::TextureTable &&tt)
    : sps(MOVE(sp))
    , texture_table(MOVE(tt))
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(GfxState);
};

struct LevelState
{
  // singular light in the scene
  opengl::Color background;
  opengl::GlobalLight global_light;

  ObjCache obj_cache;
  LevelData level_data;

  Camera camera;
  WorldObject player;
  explicit LevelState(opengl::Color const& bgcolor, opengl::GlobalLight const& glight,
      ObjCache &&ocache, LevelData &&ldata, Camera &&cam, WorldObject &&pl)
    : background(bgcolor)
    , global_light(glight)
    , obj_cache(MOVE(ocache))
    , level_data(MOVE(ldata))
    , camera(MOVE(cam))
    , player(MOVE(pl))
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(LevelState);
};

// This lives here, and not in zone.hpp, to avoid circular include cyle.
struct ZoneState
{
  LevelState level_state;
  GfxState gfx_state;
  entt::DefaultRegistry &registry;

  explicit ZoneState(LevelState &&level, GfxState &&gfx, entt::DefaultRegistry &reg)
    : level_state(MOVE(level))
    , gfx_state(MOVE(gfx))
    , registry(reg)
  {
  }
  MOVE_CONSTRUCTIBLE_ONLY(ZoneState);
};
// This lives here, and not in zone.hpp, to avoid circular include cyle.
using ZoneStates = std::vector<ZoneState>;

} // ns boomhs
