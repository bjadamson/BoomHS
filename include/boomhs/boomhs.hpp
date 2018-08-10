#pragma once
#include <boomhs/state.hpp>

#include <opengl/debug_renderer.hpp>
#include <opengl/entity_renderer.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox_renderer.hpp>
#include <opengl/sun_renderer.hpp>
#include <opengl/terrain_renderer.hpp>
#include <opengl/water_renderer.hpp>

#include <stlw/result.hpp>
#include <string>

namespace opengl
{
struct DrawState;
struct RenderState;
} // namespace opengl

namespace window
{
class FrameTime;
}

namespace stlw
{
class float_generator;
} // namespace stlw

namespace boomhs
{
class Camera;

Result<GameState, std::string>
init(Engine&, EngineState&, Camera&, stlw::float_generator&);

struct StaticRenderers;
void
game_loop(Engine&, GameState&, StaticRenderers&, stlw::float_generator&, Camera&,
          window::FrameTime const&);

struct WaterRenderers
{
  opengl::BasicWaterRenderer    basic;
  opengl::MediumWaterRenderer   medium;
  opengl::AdvancedWaterRenderer advanced;

  opengl::SilhouetteWaterRenderer silhouette;

  void render(opengl::RenderState&, opengl::DrawState&, LevelManager&, Camera&,
              window::FrameTime const&, bool);
};

struct StaticRenderers
{
  opengl::DefaultTerrainRenderer    default_terrain;
  opengl::SilhouetteTerrainRenderer silhouette_terrain;

  opengl::EntityRenderer           default_entity;
  opengl::SilhouetteEntityRenderer silhouette_entity;

  opengl::SkyboxRenderer   skybox;
  opengl::SunshaftRenderer sunshaft;

  opengl::DebugRenderer debug;

  WaterRenderers water;

  NO_COPY_OR_MOVE(StaticRenderers);
  void render(LevelManager&, opengl::RenderState&, stlw::float_generator&, opengl::DrawState&,
              window::FrameTime const&, bool);
};

StaticRenderers
make_static_renderers(EngineState&, LevelManager&);

} // namespace boomhs
