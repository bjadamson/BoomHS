#pragma once
#include <opengl/debug_renderer.hpp>
#include <opengl/entity_renderer.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox_renderer.hpp>
#include <opengl/sun_renderer.hpp>
#include <opengl/terrain_renderer.hpp>
#include <opengl/water_renderer.hpp>

#include <opengl/ui_renderer.hpp>

namespace opengl
{
struct DrawState;
struct RenderState;
} // namespace opengl

namespace boomhs
{
class Camera;
class FrameTime;
class LevelManager;
class RNG;
struct ZoneState;



struct StaticRenderers
{
  NOCOPY_MOVE_DEFAULT(StaticRenderers);

  opengl::DefaultTerrainRenderer    default_terrain;
  opengl::SilhouetteTerrainRenderer silhouette_terrain;

  opengl::EntityRenderer           default_entity;
  opengl::SilhouetteEntityRenderer silhouette_entity;

  opengl::SkyboxRenderer   skybox;
  opengl::SunshaftRenderer sunshaft;

  opengl::DebugRenderer debug;
  opengl::WaterRenderers        water;

  opengl::static_shaders::BasicMvWithUniformColor color2d;

  void render(LevelManager&, opengl::RenderState&, Camera&, RNG&, opengl::DrawState&,
              FrameTime const&, bool);
};

StaticRenderers
make_static_renderers(EngineState&, ZoneState&, Viewport const&);

} // namespace boomhs
