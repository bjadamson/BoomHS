#pragma once
#include <boomhs/camera.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/screen_info.hpp>

#include <opengl/bind.hpp>
#include <opengl/framebuffer.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/skybox_renderer.hpp>
#include <opengl/texture.hpp>

#include <common/log.hpp>
#include <common/type_macros.hpp>

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class RNG;
} // namespace boomhs

namespace opengl
{

static float constexpr CUTOFF_HEIGHT      = 0.4f;
static glm::vec4 constexpr ABOVE_VECTOR   = {0, -1, 0, CUTOFF_HEIGHT};
static glm::vec4 constexpr BENEATH_VECTOR = {0, 1, 0, -CUTOFF_HEIGHT};

class SilhouetteWaterRenderer
{
  common::Logger&        logger_;
  opengl::ShaderProgram& sp_;

public:
  SilhouetteWaterRenderer(common::Logger&, opengl::ShaderProgram&);
  MOVE_CONSTRUCTIBLE_ONLY(SilhouetteWaterRenderer);

  void render_water(RenderState&, DrawState&, boomhs::LevelManager&, boomhs::Camera&,
                    window::FrameTime const&);
};

class BasicWaterRenderer
{
  common::Logger&        logger_;
  opengl::ShaderProgram& sp_;
  opengl::TextureInfo&   diffuse_;
  opengl::TextureInfo&   normal_;

public:
  BasicWaterRenderer(common::Logger&, opengl::TextureInfo&, opengl::TextureInfo&,
                     opengl::ShaderProgram&);
  MOVE_CONSTRUCTIBLE_ONLY(BasicWaterRenderer);

  void render_water(RenderState&, DrawState&, boomhs::LevelManager&, boomhs::Camera&,
                    window::FrameTime const&);
};

class MediumWaterRenderer
{
  common::Logger&        logger_;
  opengl::ShaderProgram& sp_;
  opengl::TextureInfo&   diffuse_;
  opengl::TextureInfo&   normal_;

public:
  MediumWaterRenderer(common::Logger&, opengl::TextureInfo&, opengl::TextureInfo&,
                      opengl::ShaderProgram&);
  MOVE_CONSTRUCTIBLE_ONLY(MediumWaterRenderer);

  void render_water(RenderState&, DrawState&, boomhs::LevelManager&, boomhs::Camera&,
                    window::FrameTime const&);
};

struct ReflectionBuffers
{
  opengl::FrameBuffer  fbo;
  opengl::TextureInfo  tbo;
  opengl::RenderBuffer rbo;

  ReflectionBuffers(common::Logger&, boomhs::ScreenSize const&);

  NO_COPY(ReflectionBuffers);
  MOVE_DEFAULT(ReflectionBuffers);
};

struct RefractionBuffers
{
  opengl::FrameBuffer fbo;
  opengl::TextureInfo tbo;
  opengl::TextureInfo dbo;

  RefractionBuffers(common::Logger&, boomhs::ScreenSize const&);

  NO_COPY(RefractionBuffers);
  MOVE_DEFAULT(RefractionBuffers);
};

class AdvancedWaterRenderer
{
  opengl::ShaderProgram& sp_;
  opengl::TextureInfo&   diffuse_;
  opengl::TextureInfo&   dudv_;
  opengl::TextureInfo&   normal_;

  ReflectionBuffers reflection_;
  RefractionBuffers refraction_;

  template <typename FN>
  void with_reflection_fbo(common::Logger& logger, FN const& fn)
  {
    reflection_.fbo->while_bound(logger, fn);
  }

  template <typename FN>
  void with_refraction_fbo(common::Logger& logger, FN const& fn)
  {
    refraction_.fbo->while_bound(logger, fn);
  }

  template <typename TerrainRenderer, typename EntityRenderer>
  void advanced_common(RenderState& rstate, boomhs::EngineState& es, boomhs::LevelManager& lm,
                       DrawState& ds, EntityRenderer& er, SkyboxRenderer& sr, TerrainRenderer& tr,
                       boomhs::RNG& rng, window::FrameTime const& ft)
  {
    auto&       zs       = lm.active();
    auto const& ldata    = zs.level_data;
    auto&       registry = zs.registry;

    auto const& fog_color = ldata.fog.color;
    render::clear_screen(fog_color);

    if (es.draw_terrain) {
      auto const& material_table = ldata.material_table;
      tr.render(rstate, material_table, registry, ft, ABOVE_VECTOR);
    }
    if (es.draw_3d_entities) {
      er.render3d(rstate, rng, ft);
    }
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(AdvancedWaterRenderer);

  explicit AdvancedWaterRenderer(common::Logger&, boomhs::ScreenSize const&, ShaderProgram&,
                                 TextureInfo&, TextureInfo&, TextureInfo&);

  template <typename TerrainRenderer, typename EntityRenderer>
  void render_reflection(boomhs::EngineState& es, DrawState& ds, boomhs::LevelManager& lm,
                         boomhs::Camera& camera, EntityRenderer& er, SkyboxRenderer& sr,
                         TerrainRenderer& tr, boomhs::RNG& rng, window::FrameTime const& ft)
  {
    auto&       logger    = es.logger;
    auto&       zs        = lm.active();
    auto&       ldata     = zs.level_data;
    auto&       registry  = zs.registry;
    auto const& fog_color = ldata.fog.color;

    // Compute the camera position beneath the water for capturing the reflective image the camera
    // will see.
    //
    // By inverting the camera's Y position before computing the view matrices, we can render the
    // world as if the camera was beneath the water's surface. This is how computing the reflection
    // texture works.
    glm::vec3 camera_pos = camera.world_position();
    camera_pos.y         = -camera_pos.y;

    auto const cstate = boomhs::CameraFrameState::from_camera_withposition(camera, camera_pos);
    boomhs::FrameState fstate{cstate, es, zs};
    RenderState        rstate{fstate, ds};

    with_reflection_fbo(logger,
                        [&]() { advanced_common(rstate, es, lm, ds, er, sr, tr, rng, ft); });
  }

  template <typename TerrainRenderer, typename EntityRenderer>
  void render_refraction(boomhs::EngineState& es, DrawState& ds, boomhs::LevelManager& lm,
                         boomhs::Camera& camera, EntityRenderer& er, SkyboxRenderer& sr,
                         TerrainRenderer& tr, boomhs::RNG& rng, window::FrameTime const& ft)
  {
    auto&       zs        = lm.active();
    auto&       logger    = es.logger;
    auto&       ldata     = zs.level_data;
    auto&       registry  = zs.registry;
    auto const& fog_color = ldata.fog.color;

    auto const         cstate = boomhs::CameraFrameState::from_camera(camera);
    boomhs::FrameState fstate{cstate, es, zs};
    RenderState        rstate{fstate, ds};

    with_refraction_fbo(logger,
                        [&]() { advanced_common(rstate, es, lm, ds, er, sr, tr, rng, ft); });
  }

  void render_water(RenderState&, DrawState&, boomhs::LevelManager&, boomhs::Camera&,
                    window::FrameTime const&);
};

} // namespace opengl
