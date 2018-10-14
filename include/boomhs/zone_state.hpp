#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/color.hpp>
#include <boomhs/lighting.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>
#include <optional>
#include <vector>

namespace boomhs
{

struct GfxState
{
  opengl::DrawHandleManager draw_handles;
  opengl::ShaderPrograms    sps;
  opengl::TextureTable      texture_table;

  explicit GfxState(opengl::ShaderPrograms&& sp, opengl::TextureTable&& tt)
      : sps(MOVE(sp))
      , texture_table(MOVE(tt))
  {
  }

  NOCOPY_MOVE_DEFAULT(GfxState);
};

// This lives here, and not in zone.hpp, to avoid circular include cyle.
struct ZoneState
{
  LevelData       level_data;
  GfxState        gfx_state;
  EntityRegistry& registry;

  explicit ZoneState(LevelData&& ldata, GfxState&& gfx, EntityRegistry& reg)
      : level_data(MOVE(ldata))
      , gfx_state(MOVE(gfx))
      , registry(reg)
  {
  }
  NOCOPY_MOVE_DEFAULT(ZoneState);
};
// This lives here, and not in zone.hpp, to avoid circular include cyle.
using ZoneStates = std::vector<ZoneState>;

} // namespace boomhs
