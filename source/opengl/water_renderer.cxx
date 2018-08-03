#include <opengl/water_renderer.hpp>
#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/material.hpp>
#include <boomhs/mesh.hpp>
#include <boomhs/state.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/water.hpp>

#include <window/timer.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>

#include <cassert>
#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

void
setup(stlw::Logger& logger, TextureInfo& ti, GLint const v)
{
  glActiveTexture(v);
  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
}

template <typename FN>
void
render_water_common(Transform& transform, ShaderProgram& sp, RenderState& rstate, DrawState& ds,
                    LevelManager& lm, Camera& camera, FrameTime const& ft, FN const& fn)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;

  auto& logger   = es.logger;
  auto& zs       = lm.active();
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;

  auto const render = [&](WaterInfo& winfo) {
    auto const& pos = winfo.position;

    auto& tr = transform.translation;
    tr.x     = pos.x;
    tr.z     = pos.y;

    // hack
    tr.y = 0.19999f; // pos.y;
    assert(tr.y < 2.0f);

    winfo.wave_offset += ft.delta_seconds() * winfo.wind_speed;
    winfo.wave_offset = ::fmodf(winfo.wave_offset, 1.00f);

    auto& time_offset = ldata.time_offset;
    time_offset += ft.delta_seconds() * winfo.wind_speed;
    time_offset = ::fmodf(time_offset, 1.00f);

    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;
    auto& vao   = dinfo.vao();

    sp.while_bound(logger, [&]() {
      sp.set_uniform_vec4(logger, "u_clipPlane", ABOVE_VECTOR);
      sp.set_uniform_float1(logger, "u_time_offset", time_offset);
      sp.set_uniform_vec2(logger, "u_flowdir", winfo.flow_direction);

      vao.while_bound(logger, [&]() { fn(winfo); });
    });
  };

  LOG_TRACE("Rendering water");
  auto const winfos = find_all_entities_with_component<WaterInfo>(registry);
  for (auto const weid : winfos) {
    auto& wi = registry.get<WaterInfo>(weid);
    render(wi);
  }
  LOG_TRACE("Finished rendering water");
}

} // namespace

namespace opengl
{

ShaderProgram&
draw_water_options_to_shader(GameGraphicsMode const dwo, opengl::ShaderPrograms& sps)
{
  ShaderProgram* sp = nullptr;

  switch (dwo) {
  case GameGraphicsMode::Basic:
    sp = &sps.ref_sp("water_basic");
    break;
  case GameGraphicsMode::Medium:
    sp = &sps.ref_sp("water_medium");
    break;
  case GameGraphicsMode::Advanced:
    sp = &sps.ref_sp("water_advanced");
    break;
  default:
    std::abort();
  }

  assert(sp);
  return *sp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BasicWaterRenderer
BasicWaterRenderer::BasicWaterRenderer(stlw::Logger& logger, TextureInfo& diff, TextureInfo& norm,
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

  Transform  transform;
  auto const fn = [&](WaterInfo& winfo) {
    sp_.set_uniform_color(logger, "u_water.mix_color", winfo.mix_color);
    sp_.set_uniform_float1(logger, "u_water.mix_intensity", winfo.mix_intensity);

    glActiveTexture(GL_TEXTURE0);
    bind::global_bind(logger, diffuse_);

    glActiveTexture(GL_TEXTURE1);
    bind::global_bind(logger, normal_);

    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;

    auto const model_matrix = transform.model_matrix();
    render::draw_3dshape(rstate, GL_TRIANGLE_STRIP, model_matrix, sp_, dinfo);

    bind::global_unbind(logger, diffuse_);
    bind::global_unbind(logger, normal_);
    glActiveTexture(GL_TEXTURE0);
  };

  render_water_common(transform, sp_, rstate, ds, lm, camera, ft, fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// MediumWaterRenderer
MediumWaterRenderer::MediumWaterRenderer(stlw::Logger& logger, TextureInfo& diff, TextureInfo& norm,
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

  Transform      transform;
  Material const water_material{};

  auto const fn = [&](WaterInfo& winfo) {
    sp_.set_uniform_color(logger, "u_water.mix_color", winfo.mix_color);
    sp_.set_uniform_float1(logger, "u_water.mix_intensity", winfo.mix_intensity);

    glActiveTexture(GL_TEXTURE0);
    bind::global_bind(logger, diffuse_);

    glActiveTexture(GL_TEXTURE1);
    bind::global_bind(logger, normal_);

    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;

    bool constexpr SET_NORMALMATRIX = false;
    auto const model_matrix         = transform.model_matrix();
    render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, transform.translation, model_matrix, sp_,
                             dinfo, water_material, registry, SET_NORMALMATRIX);

    bind::global_unbind(logger, diffuse_);
    bind::global_unbind(logger, normal_);
    glActiveTexture(GL_TEXTURE0);
  };

  render_water_common(transform, sp_, rstate, ds, lm, camera, ft, fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReflectionBuffers
ReflectionBuffers::ReflectionBuffers(stlw::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{opengl::make_fbo(logger, ss)})
    , rbo(RBInfo{})
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RefractionBuffers
RefractionBuffers::RefractionBuffers(stlw::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{opengl::make_fbo(logger, ss)})
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AdvancedWaterRenderer
AdvancedWaterRenderer::AdvancedWaterRenderer(stlw::Logger& logger, ScreenSize const& screen_size,
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
    auto const size = reflection_.fbo->dimensions.size();
    auto const w = size.first, h = size.second;

    with_reflection_fbo(logger, [&]() {
      reflection_.tbo = create_texture_attachment(logger, w, h, GL_TEXTURE1);
      reflection_.rbo = create_depth_buffer_attachment(logger, w, h);
    });
  }

  {
    // TODO: structured binding bug
    auto const size = refraction_.fbo->dimensions.size();
    auto const w = size.first, h = size.second;

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
  Transform      transform;

  auto const fn = [&](WaterInfo& winfo) {
    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;

    sp_.set_uniform_vec3(logger, "u_camera_position", camera.world_position());
    sp_.set_uniform_float1(logger, "u_wave_offset", winfo.wave_offset);
    sp_.set_uniform_float1(logger, "u_wavestrength", winfo.wave_strength);

    sp_.set_uniform_color(logger, "u_water.mix_color", winfo.mix_color);
    sp_.set_uniform_float1(logger, "u_water.mix_intensity", winfo.mix_intensity);

    glActiveTexture(GL_TEXTURE0);
    bind::global_bind(logger, diffuse_);

    glActiveTexture(GL_TEXTURE1);
    bind::global_bind(logger, reflection_.tbo);
    bind::global_bind(logger, reflection_.rbo.resource());

    glActiveTexture(GL_TEXTURE2);
    bind::global_bind(logger, refraction_.tbo);

    glActiveTexture(GL_TEXTURE3);
    bind::global_bind(logger, dudv_);

    glActiveTexture(GL_TEXTURE4);
    bind::global_bind(logger, normal_);

    glActiveTexture(GL_TEXTURE5);
    bind::global_bind(logger, refraction_.dbo);

    ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
    bool constexpr SET_NORMALMATRIX = false;
    auto const model_matrix         = transform.model_matrix();
    render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, transform.translation, model_matrix, sp_,
                             dinfo, water_material, registry, SET_NORMALMATRIX);
    bind::global_unbind(logger, diffuse_);
    bind::global_unbind(logger, reflection_.tbo);
    bind::global_unbind(logger, reflection_.rbo.resource());
    bind::global_unbind(logger, refraction_.tbo);
    bind::global_unbind(logger, dudv_);
    bind::global_unbind(logger, normal_);
    bind::global_unbind(logger, refraction_.dbo);

    glActiveTexture(GL_TEXTURE0);
  };
  render_water_common(transform, sp_, rstate, ds, lm, camera, ft, fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BlackWaterRenderer
BlackWaterRenderer::BlackWaterRenderer(stlw::Logger& logger, ShaderProgram& sp)
    : logger_(logger)
    , sp_(sp)
{
}

void
BlackWaterRenderer::render_water(RenderState& rstate, DrawState& ds, LevelManager& lm,
                                 Camera& camera, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;

  auto& logger = es.logger;

  Transform  transform;
  auto const fn = [&](WaterInfo& winfo) {
    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;

    auto const model_matrix = transform.model_matrix();
    render::draw_3dblack_water(rstate, GL_TRIANGLE_STRIP, model_matrix, sp_, dinfo);
  };

  render_water_common(transform, sp_, rstate, ds, lm, camera, ft, fn);
}

} // namespace opengl
