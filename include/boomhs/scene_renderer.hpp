#pragma once
#include <opengl/debug_renderer.hpp>
#include <opengl/entity_renderer.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox_renderer.hpp>
#include <opengl/sun_renderer.hpp>
#include <opengl/terrain_renderer.hpp>
#include <opengl/water_renderer.hpp>

namespace opengl
{
struct DrawState;
class RenderState;
} // namespace opengl

namespace boomhs
{
class  Camera;
class  FrameTime;
class  LevelManager;
class  RNG;
struct ZoneState;

struct WaterRenderers
{
  opengl::BasicWaterRenderer    basic;
  opengl::MediumWaterRenderer   medium;
  opengl::AdvancedWaterRenderer advanced;

  opengl::SilhouetteWaterRenderer silhouette;

  void render(opengl::RenderState&, opengl::DrawState&, LevelManager&, Camera&,
              FrameTime const&, bool);
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
  void render(LevelManager&, opengl::RenderState&, Camera&, RNG&, opengl::DrawState&,
              FrameTime const&, bool);
};

StaticRenderers
make_static_renderers(EngineState&, ZoneState&);

} // namespace boomhs
