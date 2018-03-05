#pragma once
#include <boomhs/camera.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/world_object.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/nearby_targets.hpp>

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

// This lives here, and not in zone.hpp, to avoid circular include cyle.
struct ZoneState
{
  LevelData level_data;
  GfxState gfx_state;
  EntityRegistry &registry;

  explicit ZoneState(LevelData &&ldata, GfxState &&gfx, EntityRegistry &reg)
    : level_data(MOVE(ldata))
    , gfx_state(MOVE(gfx))
    , registry(reg)
  {
  }
  MOVE_CONSTRUCTIBLE_ONLY(ZoneState);
};
// This lives here, and not in zone.hpp, to avoid circular include cyle.
using ZoneStates = std::vector<ZoneState>;

} // ns boomhs