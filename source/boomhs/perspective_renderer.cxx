#include <boomhs/perspective_renderer.hpp>
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
#include <boomhs/frame_time.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/item.hpp>
#include <boomhs/item_factory.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/rexpaint.hpp>
#include <boomhs/state.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/water.hpp>


#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
#include <opengl/texture.hpp>

#include <extlibs/sdl.hpp>

#include <common/log.hpp>
#include <boomhs/math.hpp>
#include <boomhs/random.hpp>
#include <common/result.hpp>

#include <extlibs/fastnoise.hpp>
#include <extlibs/imgui.hpp>

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
using namespace gl_sdl;

namespace
{

} // namespace

namespace boomhs
{

void
PerspectiveRenderer::draw_scene(RenderState& rstate, LevelManager& lm, DrawState& ds, Camera& camera,
             RNG& rng, StaticRenderers& static_renderers, FrameTime const& ft)
{
  auto& es     = rstate.fs.es;
  auto& logger = es.logger;
  auto const& graphics_settings      = es.graphics_settings;
  bool const  graphics_mode_advanced = GameGraphicsMode::Advanced == graphics_settings.mode;

  auto const& water_buffer = es.ui_state.debug.buffers.water;
  bool const  draw_water = water_buffer.draw;
  bool const  draw_water_advanced    = draw_water && graphics_mode_advanced;

  auto& skybox_renderer = static_renderers.skybox;

  auto const draw_scene = [&](bool const silhouette_black) {
    auto& water_renderer = static_renderers.water;
    auto const draw_advanced = [&](auto& terrain_renderer, auto& entity_renderer) {
      water_renderer.advanced.render_reflection(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, rng, ft);
      water_renderer.advanced.render_refraction(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, rng, ft);
    };
    if (draw_water && draw_water_advanced && !silhouette_black) {
      // Render the scene to the refraction and reflection FBOs
      draw_advanced(static_renderers.default_terrain, static_renderers.default_entity);
    }

    auto const& zs    = lm.active();
    auto const& ldata = zs.level_data;
    auto const clear_color = silhouette_black ? LOC::BLACK : ldata.fog.color;
    render::clear_screen(clear_color);

    // render scene
    if (es.draw_skybox) {
      if (!silhouette_black) {
        skybox_renderer.render(rstate, ds, ft);
      }
    }

    // The water must be drawn BEFORE rendering the scene the last time, otherwise it shows up
    // ontop of the ingame UI nearby target indicators.
    if (draw_water) {
      water_renderer.render(rstate, ds, lm, camera, ft, silhouette_black);
    }

    static_renderers.render(lm, rstate, camera, rng, ds, ft, silhouette_black);
  };

  auto const draw_scene_normal_render = [&]() { draw_scene(false); };

  auto const render_scene_with_sunshafts = [&]() {
    // draw scene with black silhouttes into the sunshaft FBO.
    auto& sunshaft_renderer = static_renderers.sunshaft;
    sunshaft_renderer.with_sunshaft_fbo(logger, [&]() { draw_scene(true); });

    // draw the scene (normal render) to the screen
    draw_scene_normal_render();

    // With additive blending enabled, render the FBO ontop of the previously rendered scene to
    // obtain the sunglare effect.
    ENABLE_ADDITIVE_BLENDING_UNTIL_SCOPE_EXIT();
    sunshaft_renderer.render(rstate, ds, lm, camera, ft);
  };

  if (!graphics_settings.disable_sunshafts) {
    render_scene_with_sunshafts();
  }
  else {
    draw_scene_normal_render();
  }
}

} // namespace boomhs
