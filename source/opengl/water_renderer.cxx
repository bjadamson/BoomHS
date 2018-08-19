#include <opengl/water_renderer.hpp>
#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/material.hpp>
#include <boomhs/mesh.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/water.hpp>
#include <boomhs/view_frustum.hpp>

#include <boomhs/clock.hpp> 
#include <common/log.hpp>
#include <boomhs/random.hpp>

#include <cassert>
#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

void
setup(common::Logger& logger, TextureInfo& ti, GLint const v)
{
  glActiveTexture(v);
  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
}

template <typename FN>
void
render_water_common(ShaderProgram& sp, RenderState& rstate, DrawState& ds,
                    LevelManager& lm, Camera& camera, FrameTime const& ft, FN const& fn)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;

  auto& logger   = es.logger;
  auto& zs       = lm.active();
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;
  auto const& wind = ldata.wind;

  auto const render = [&](WaterInfo& winfo) {
    auto const eid = winfo.eid;
    if (!registry.get<IsVisible>(eid).value) {
      return;
    }
    auto const &tr = registry.get<Transform>(eid);
    auto const &bbox = registry.get<AABoundingBox>(eid);

    glm::mat4 const& view_mat = fstate.view_matrix();
    glm::mat4 const& proj_mat = fstate.projection_matrix();
    if (!ViewFrustum::bbox_inside(view_mat, proj_mat, tr, bbox)) {
      return;
    }

    winfo.wave_offset += ft.delta_seconds() * wind.speed;
    winfo.wave_offset = ::fmodf(winfo.wave_offset, 1.00f);

    auto& time_offset = ldata.time_offset;
    time_offset += ft.delta_seconds() * wind.speed;
    time_offset = ::fmodf(time_offset, 1.00f);

    auto& gfx_state    = zs.gfx_state;
    auto& draw_handles = gfx_state.draw_handles;
    auto& dinfo        = draw_handles.lookup_entity(logger, eid);

    sp.while_bound(logger, [&]() {
      sp.set_uniform_vec4(logger, "u_clipPlane", ABOVE_VECTOR);
      sp.set_uniform_float1(logger, "u_time_offset", time_offset);
      sp.set_uniform_vec2(logger, "u_flowdir", winfo.flow_direction);

      auto& wbuffer = es.ui_state.debug.buffers.water;
      sp.set_uniform_float1(logger, "u_water.weight_light",      wbuffer.weight_light);
      sp.set_uniform_float1(logger, "u_water.weight_texture",    wbuffer.weight_texture);
      sp.set_uniform_float1(logger, "u_water.weight_mix_effect", wbuffer.weight_mix_effect);

      dinfo.while_bound(logger, [&]() { fn(winfo, tr); });
    });
  };

  LOG_TRACE("Rendering water");
  auto const winfos = find_all_entities_with_component<WaterInfo>(registry);
  for (auto const eid : winfos) {
    auto& wi = registry.get<WaterInfo>(eid);
    render(wi);
  }
  LOG_TRACE("Finished rendering water");
}

} // namespace

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// BasicWaterRenderer
BasicWaterRenderer::BasicWaterRenderer(common::Logger& logger, TextureInfo& diff, TextureInfo& norm,
                                       ShaderProgram& sp)
    : logger_(logger)
    , sp_(sp)
    , diffuse_(diff)
    , normal_(norm)
{
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });
  setup(logger, diffuse_, GL_TEXTURE0);
  setup(logger, normal_, GL_TEXTURE1);

  // connect texture units to shader program
  sp_.while_bound(logger, [&]() {
    sp_.set_uniform_int1(logger, "u_diffuse_sampler", 0);
    sp_.set_uniform_int1(logger, "u_normal_sampler", 1);
  });
}

void
BasicWaterRenderer::render_water(RenderState& rstate, DrawState& ds, LevelManager& lm,
                                 Camera& camera, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto const fn = [&](WaterInfo& winfo, Transform const& transform) {
    sp_.set_uniform_color(logger, "u_water.mix_color", winfo.mix_color);
    sp_.set_uniform_float1(logger, "u_water.mix_intensity", winfo.mix_intensity);

    glActiveTexture(GL_TEXTURE0);
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

    BIND_UNTIL_END_OF_SCOPE(logger, diffuse_);

    glActiveTexture(GL_TEXTURE1);
    BIND_UNTIL_END_OF_SCOPE(logger, normal_);

    auto& zs       = lm.active();
    auto&       gfx_state = zs.gfx_state;
    auto& draw_handles = gfx_state.draw_handles;
    auto& dinfo = draw_handles.lookup_entity(logger, winfo.eid);

    auto const model_matrix = transform.model_matrix();
    render::draw_3dshape(rstate, GL_TRIANGLE_STRIP, model_matrix, sp_, dinfo);
  };

  render_water_common(sp_, rstate, ds, lm, camera, ft, fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// MediumWaterRenderer
MediumWaterRenderer::MediumWaterRenderer(common::Logger& logger, TextureInfo& diff, TextureInfo& norm,
                                         ShaderProgram& sp)
    : logger_(logger)
    , sp_(sp)
    , diffuse_(diff)
    , normal_(norm)
{
  {
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });
    setup(logger, diffuse_, GL_TEXTURE0);
    setup(logger, normal_, GL_TEXTURE1);
  }

  // connect texture units to shader program
  sp_.while_bound(logger, [&]() {
    sp_.set_uniform_int1(logger, "u_diffuse_sampler", 0);
    sp_.set_uniform_int1(logger, "u_normal_sampler", 1);
  });
}

void
MediumWaterRenderer::render_water(RenderState& rstate, DrawState& ds, LevelManager& lm,
                                  Camera& camera, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;

  auto& logger   = es.logger;
  auto& zs       = lm.active();
  auto& registry = zs.registry;

  Material const water_material{};

  auto const fn = [&](WaterInfo& winfo, Transform const& transform) {
    sp_.set_uniform_color(logger, "u_water.mix_color", winfo.mix_color);
    sp_.set_uniform_float1(logger, "u_water.mix_intensity", winfo.mix_intensity);

    glActiveTexture(GL_TEXTURE0);
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

    BIND_UNTIL_END_OF_SCOPE(logger, diffuse_);

    glActiveTexture(GL_TEXTURE1);
    BIND_UNTIL_END_OF_SCOPE(logger, normal_);

    auto&       gfx_state = zs.gfx_state;
    auto& draw_handles = gfx_state.draw_handles;
    auto& dinfo = draw_handles.lookup_entity(logger, winfo.eid);

    bool constexpr SET_NORMALMATRIX = false;
    auto const model_matrix         = transform.model_matrix();
    render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, transform.translation, model_matrix, sp_,
                             dinfo, water_material, registry, SET_NORMALMATRIX);
  };

  render_water_common(sp_, rstate, ds, lm, camera, ft, fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReflectionBuffers
ReflectionBuffers::ReflectionBuffers(common::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{opengl::make_fbo(logger, ss)})
    , rbo(RBInfo{})
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RefractionBuffers
RefractionBuffers::RefractionBuffers(common::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{opengl::make_fbo(logger, ss)})
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AdvancedWaterRenderer
AdvancedWaterRenderer::AdvancedWaterRenderer(common::Logger& logger, ScreenSize const& screen_size,
                                             ShaderProgram& sp, TextureInfo& diffuse,
                                             TextureInfo& dudv, TextureInfo& normal)
    : sp_(sp)
    , diffuse_(diffuse)
    , dudv_(dudv)
    , normal_(normal)
    , reflection_(logger, screen_size)
    , refraction_(logger, screen_size)
{
  {
    // TODO: structured binding bug
    auto const dim = reflection_.fbo->dimensions;
    auto const w = dim.width(), h = dim.height();

    with_reflection_fbo(logger, [&]() {
      reflection_.tbo = create_texture_attachment(logger, w, h, GL_TEXTURE1);
      reflection_.rbo = create_depth_buffer_attachment(logger, w, h);
    });
  }

  {
    // TODO: structured binding bug
    auto const dim = refraction_.fbo->dimensions;
    auto const w = dim.width(), h = dim.height();

    with_refraction_fbo(logger, [&]() {
      GLenum const tu = GL_TEXTURE2;
      refraction_.tbo = create_texture_attachment(logger, w, h, tu);
      refraction_.dbo = create_depth_texture_attachment(logger, w, h, tu);
    });
  }

  {
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });
    setup(logger, diffuse_, GL_TEXTURE0);
    setup(logger, reflection_.tbo, GL_TEXTURE1);
    setup(logger, refraction_.tbo, GL_TEXTURE2);
    setup(logger, dudv_, GL_TEXTURE3);
    setup(logger, normal_, GL_TEXTURE4);
    setup(logger, refraction_.dbo, GL_TEXTURE5);
  }

  // connect texture units to shader program
  sp_.while_bound(logger, [&]() {
    sp_.set_uniform_int1(logger, "u_diffuse_sampler", 0);
    sp_.set_uniform_int1(logger, "u_reflect_sampler", 1);
    sp_.set_uniform_int1(logger, "u_refract_sampler", 2);
    sp_.set_uniform_int1(logger, "u_dudv_sampler", 3);
    sp_.set_uniform_int1(logger, "u_normal_sampler", 4);
    sp_.set_uniform_int1(logger, "u_depth_sampler", 5);
  });
}

void
AdvancedWaterRenderer::render_water(RenderState& rstate, DrawState& ds, LevelManager& lm,
                                    Camera& camera, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;

  auto& logger   = es.logger;
  auto& zs       = lm.active();
  auto& registry = zs.registry;

  Material const water_material{};

  auto const fn = [&](WaterInfo& winfo, Transform const& transform) {

    auto&       gfx_state = zs.gfx_state;
    auto& draw_handles = gfx_state.draw_handles;
    auto& dinfo = draw_handles.lookup_entity(logger, winfo.eid);

    sp_.set_uniform_vec3(logger, "u_camera_position", camera.world_position());
    sp_.set_uniform_float1(logger, "u_wave_offset", winfo.wave_offset);
    sp_.set_uniform_float1(logger, "u_wavestrength", winfo.wave_strength);

    sp_.set_uniform_color(logger, "u_water.mix_color", winfo.mix_color);
    sp_.set_uniform_float1(logger, "u_water.mix_intensity", winfo.mix_intensity);

    glActiveTexture(GL_TEXTURE0);
    ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

    BIND_UNTIL_END_OF_SCOPE(logger, diffuse_);

    glActiveTexture(GL_TEXTURE1);
    BIND_UNTIL_END_OF_SCOPE(logger, reflection_.tbo);
    BIND_UNTIL_END_OF_SCOPE(logger, reflection_.rbo.resource());

    glActiveTexture(GL_TEXTURE2);
    BIND_UNTIL_END_OF_SCOPE(logger, refraction_.tbo);

    glActiveTexture(GL_TEXTURE3);
    BIND_UNTIL_END_OF_SCOPE(logger, dudv_);

    glActiveTexture(GL_TEXTURE4);
    BIND_UNTIL_END_OF_SCOPE(logger, normal_);

    glActiveTexture(GL_TEXTURE5);
    BIND_UNTIL_END_OF_SCOPE(logger, refraction_.dbo);

    ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
    bool constexpr SET_NORMALMATRIX = false;
    auto const model_matrix         = transform.model_matrix();
    render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, transform.translation, model_matrix, sp_,
                             dinfo, water_material, registry, SET_NORMALMATRIX);
  };
  render_water_common(sp_, rstate, ds, lm, camera, ft, fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// SilhouetteWaterRenderer
SilhouetteWaterRenderer::SilhouetteWaterRenderer(common::Logger& logger, ShaderProgram& sp)
    : logger_(logger)
    , sp_(sp)
{
}

void
SilhouetteWaterRenderer::render_water(RenderState& rstate, DrawState& ds, LevelManager& lm,
                                 Camera& camera, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;

  auto& logger = es.logger;

  auto const fn = [&](WaterInfo& winfo, Transform const& transform) {

    auto& zs       = lm.active();
    auto&       gfx_state = zs.gfx_state;
    auto& draw_handles = gfx_state.draw_handles;
    auto& dinfo = draw_handles.lookup_entity(logger, winfo.eid);

    auto const model_matrix = transform.model_matrix();
    render::draw_3dblack_water(rstate, GL_TRIANGLE_STRIP, model_matrix, sp_, dinfo);
  };

  render_water_common(sp_, rstate, ds, lm, camera, ft, fn);
}

} // namespace opengl
