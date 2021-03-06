#include <boomhs/scene_renderer.hpp>

#include <boomhs/audio.hpp>
#include <boomhs/billboard.hpp>
#include <boomhs/boomhs.hpp>
#include <boomhs/bounding_object.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/controller.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/item.hpp>
#include <boomhs/item_factory.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/math.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/random.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/state.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/vertex_factory.hpp>
#include <boomhs/water.hpp>
#include <boomhs/zone_state.hpp>

#include <opengl/gpu.hpp>
#include <opengl/texture.hpp>

#include <common/log.hpp>
#include <common/result.hpp>

#include <extlibs/fastnoise.hpp>
#include <extlibs/imgui.hpp>
#include <extlibs/sdl.hpp>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

using namespace boomhs;
using namespace boomhs::math::constants;
using namespace opengl;

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// StaticRenderers
void
StaticRenderers::render(LevelManager& lm, RenderState& rstate, Camera& camera, RNG& rng,
                        DrawState& ds, FrameTime const& ft, bool const silhouette_black)
{
  // Render the scene with no culling (setting it zero disables culling mathematically)
  glm::vec4 const NOCULL_VECTOR{0, 0, 0, 0};

  auto& fs = rstate.fs;
  auto& es = fs.es;
  if (es.draw_terrain) {
    auto const draw_basic = [&](auto& terrain_renderer, auto& entity_renderer) {
      auto& zs       = fs.zs;
      auto& ldata    = zs.level_data;
      auto& registry = zs.registry;

      terrain_renderer.render(rstate, ldata.material_table, registry, ft, NOCULL_VECTOR);
    };
    if (silhouette_black) {
      draw_basic(silhouette_terrain, silhouette_entity);
    }
    else {
      draw_basic(default_terrain, default_entity);
    }
  }
  // DRAW ALL ENTITIESImGuiWindowFlags_AlwaysAutoResize
  {
    if (es.draw_3d_entities) {
      if (silhouette_black) {
        silhouette_entity.render3d(rstate, rng, ft);
      }
      else {
        default_entity.render3d(rstate, rng, ft);
      }
    }
    if (es.draw_2d_billboard_entities) {
      if (silhouette_black) {
        silhouette_entity.render2d_billboard(rstate, rng, ft);
      }
      else {
        default_entity.render2d_billboard(rstate, rng, ft);
      }
    }
    if (es.draw_2d_ui_entities) {
      if (silhouette_black) {
        silhouette_entity.render2d_ui(rstate, rng, ft);
      }
      else {
        default_entity.render2d_ui(rstate, rng, ft);
      }
    }
  }
  if (silhouette_black) {
    // do nothing
  }
  else {
    debug.render_scene(rstate, lm, camera, rng, ft);
  }
}

StaticRenderers
make_static_renderers(EngineState& es, ZoneState& zs, Viewport const& viewport)
{
  auto const make_basic_water_renderer = [](common::Logger& logger, ShaderPrograms& sps,
                                            TextureTable& ttable) {
    auto& diff   = *ttable.find("water-diffuse");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = graphics_mode_to_water_shader(logger, GameGraphicsMode::Basic, sps);
    return BasicWaterRenderer{logger, diff, normal, sp};
  };

  auto const make_medium_water_renderer = [](common::Logger& logger, ShaderPrograms& sps,
                                             TextureTable& ttable) {
    auto& diff   = *ttable.find("water-diffuse");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = graphics_mode_to_water_shader(logger, GameGraphicsMode::Medium, sps);
    return MediumWaterRenderer{logger, diff, normal, sp};
  };

  auto const make_advanced_water_renderer = [&](EngineState& es, ZoneState& zs) {
    auto& logger = es.logger;

    auto& gfx_state = zs.gfx_state;
    auto& ttable    = gfx_state.texture_table;
    auto& sps       = gfx_state.sps;
    auto& ti        = *ttable.find("water-diffuse");
    auto& dudv      = *ttable.find("water-dudv");
    auto& normal    = *ttable.find("water-normal");
    auto& sp        = graphics_mode_to_water_shader(logger, GameGraphicsMode::Advanced, sps);
    return AdvancedWaterRenderer{logger, viewport, sp, ti, dudv, normal};
  };

  auto const make_black_water_renderer = [](EngineState& es, ZoneState& zs) {
    auto& logger    = es.logger;
    auto& gfx_state = zs.gfx_state;
    auto& sps       = gfx_state.sps;
    auto& sp        = sps.sp_silhoutte_3d(logger);
    return SilhouetteWaterRenderer{logger, sp};
  };

  auto const make_black_terrain_renderer = [](EngineState& es, ShaderPrograms& sps) {
    auto& logger    = es.logger;
    auto& sp = sps.sp_silhoutte_3d(logger);
    return SilhouetteTerrainRenderer{sp};
  };

  auto const make_sunshaft_renderer = [&](EngineState& es, ZoneState& zs) {
    auto& logger      = es.logger;
    auto& gfx_state   = zs.gfx_state;
    auto& sps         = gfx_state.sps;
    auto& sunshaft_sp = sps.sp_sunshaft(logger);

    return SunshaftRenderer{logger, viewport, sunshaft_sp};
  };

  auto const make_skybox_renderer = [](common::Logger& logger, ShaderPrograms& sps,
                                       TextureTable& ttable) {
    auto&           skybox_sp = sps.sp_skybox(logger);
    glm::vec3 const vmin{-0.5f};
    glm::vec3 const vmax{0.5f};

    auto const vertices = VertexFactory::build_cube(vmin, vmax);
    DrawInfo   dinfo    = OG::copy_cube_gpu(logger, vertices, skybox_sp.va());
    auto&      day_ti   = *ttable.find("building_skybox");
    auto&      night_ti = *ttable.find("night_skybox");
    return SkyboxRenderer{logger, MOVE(dinfo), day_ti, night_ti, skybox_sp};
  };
  auto& logger    = es.logger;
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;
  return StaticRenderers{DefaultTerrainRenderer{},
                         make_black_terrain_renderer(es, sps),
                         EntityRenderer{},
                         SilhouetteEntityRenderer{},
                         make_skybox_renderer(logger, sps, ttable),
                         make_sunshaft_renderer(es, zs),
                         DebugRenderer{},

                         WaterRenderers{make_basic_water_renderer(logger, sps, ttable),
                                        make_medium_water_renderer(logger, sps, ttable),
                                        make_advanced_water_renderer(es, zs),
                                        make_black_water_renderer(es, zs)},
                         static_shaders::BasicMvWithUniformColor::create(logger)};
}

} // namespace boomhs
