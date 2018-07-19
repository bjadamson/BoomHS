#include <boomhs/camera.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mesh.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/water.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

#include <window/timer.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>

#include <cassert>
#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

float constexpr CUTOFF_HEIGHT      = 0.4f;
glm::vec4 constexpr ABOVE_VECTOR   = {0, -1, 0, CUTOFF_HEIGHT};
glm::vec4 constexpr BENEATH_VECTOR = {0, 1, 0, -CUTOFF_HEIGHT};

namespace
{

auto
make_fbo(stlw::Logger& logger, ScreenSize const& ss)
{
  FBInfo fb{{0, 0, ss.width, ss.height}, ss};
  fb.while_bound(logger, []() { glDrawBuffer(GL_COLOR_ATTACHMENT0); });
  return fb;
}

TextureInfo
create_texture_attachment(stlw::Logger& logger, int const width, int const height,
                          GLenum const texture_unit)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  glActiveTexture(texture_unit);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  ti.while_bound(logger, [&]() {
    // allocate memory for texture
    glTexImage2D(ti.target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // adjust texture fields
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // attach texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ti.target, ti.id, 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

auto
create_depth_texture_attachment(stlw::Logger& logger, int const width, int const height,
                                GLenum const texture_unit)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  glActiveTexture(texture_unit);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(ti.target, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ti.target, ti.id, 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

auto
create_depth_buffer_attachment(stlw::Logger& logger, int const width, int const height)
{
  RBInfo rbinfo;
  rbinfo.while_bound(logger, [&]() {
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbinfo.id);
  });
  return RenderBuffer{MOVE(rbinfo)};
}

void
setup(stlw::Logger& logger, TextureInfo& ti, GLint const v)
{
  glActiveTexture(v);
  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// WaterFactory
ObjData
WaterFactory::generate_water_data(stlw::Logger& logger, glm::vec2 const& dimensions,
                                  size_t const num_vertexes)
{
  auto const count = num_vertexes * num_vertexes;

  ObjData data;
  data.num_vertexes = count;

  data.vertices = MeshFactory::generate_rectangle_mesh(logger, dimensions, num_vertexes);

  bool constexpr TILE = true;
  data.uvs            = MeshFactory::generate_uvs(logger, dimensions, num_vertexes, TILE);

  data.indices = MeshFactory::generate_indices(logger, num_vertexes);

  return data;
}

WaterInfo
WaterFactory::generate_info(stlw::Logger& logger, TextureInfo& tinfo)
{
  WaterInfo wi{};
  wi.tinfo = &tinfo;

  return wi;
}

WaterInfo
WaterFactory::make_default(stlw::Logger& logger, ShaderPrograms& sps, TextureTable& ttable)
{
  LOG_TRACE("Generating water");

  auto texture_o = ttable.find("water-diffuse");
  assert(texture_o);
  auto& ti = *texture_o;

  auto wi = generate_info(logger, ti);

  auto& tinfo = wi.tinfo;
  tinfo->while_bound(logger, [&]() {
    tinfo->set_fieldi(GL_TEXTURE_WRAP_S, GL_REPEAT);
    tinfo->set_fieldi(GL_TEXTURE_WRAP_T, GL_REPEAT);
    tinfo->set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    tinfo->set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  });
  LOG_TRACE("Finished generating water");
  return wi;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ReflectionBuffers
ReflectionBuffers::ReflectionBuffers(stlw::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{make_fbo(logger, ss)})
    , rbo(RBInfo{})
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RefractionBuffers
RefractionBuffers::RefractionBuffers(stlw::Logger& logger, ScreenSize const& ss)
    : fbo(FrameBuffer{make_fbo(logger, ss)})
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
AdvancedWaterRenderer::render_reflection(EngineState& es, DrawState& ds, LevelManager& lm,
                                         Camera& camera, SkyboxRenderer& skybox_renderer,
                                         stlw::float_generator& rng, FrameTime const& ft)
{
  auto&       zs        = lm.active();
  auto&       logger    = es.logger;
  auto&       ldata     = zs.level_data;
  auto const& fog_color = ldata.fog.color;

  // Compute the camera position beneath the water for capturing the reflective image the camera
  // will see.
  //
  // By inverting the camera's Y position before computing the view matrices, we can render the
  // world as if the camera was beneath the water's surface. This is how computing the reflection
  // texture works.
  glm::vec3 camera_pos = camera.world_position();
  camera_pos.y         = -camera_pos.y;

  auto const  fmatrices = FrameMatrices::from_camera_withposition(camera, camera_pos);
  FrameState  fstate{fmatrices, es, zs};
  RenderState rstate{fstate, ds};

  with_reflection_fbo(logger, [&]() {
    render::clear_screen(fog_color);
    skybox_renderer.render(rstate, ds, ft);
    render::render_scene(rstate, lm, rng, ft, ABOVE_VECTOR);
  });
}

void
AdvancedWaterRenderer::render_refraction(EngineState& es, DrawState& ds, LevelManager& lm,
                                         Camera& camera, SkyboxRenderer& skybox_renderer,
                                         stlw::float_generator& rng, FrameTime const& ft)
{
  auto&       zs        = lm.active();
  auto&       logger    = es.logger;
  auto&       ldata     = zs.level_data;
  auto const& fog_color = ldata.fog.color;

  auto const  fmatrices = FrameMatrices::from_camera(camera);
  FrameState  fstate{fmatrices, es, zs};
  RenderState rstate{fstate, ds};

  with_refraction_fbo(logger, [&]() {
    render::clear_screen(fog_color);

    skybox_renderer.render(rstate, ds, ft);
    render::render_scene(rstate, lm, rng, ft, BENEATH_VECTOR);
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
  auto& ldata    = zs.level_data;

  Transform  transform;
  auto const render = [&](WaterInfo& winfo) {
    auto const& pos = winfo.position;

    auto& tr = transform.translation;
    tr.x     = pos.x;
    tr.z     = pos.y;

    // hack
    tr.y = 0.19999f; // pos.y;
    assert(tr.y < 2.0f);

    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;
    auto& vao   = dinfo.vao();

    bool constexpr RECEIVES_AMBIENT_LIGHT = true;
    bool constexpr SET_NORMALMATRIX       = false;
    auto const model_matrix               = transform.model_matrix();

    winfo.wave_offset += ft.delta_millis() * ldata.wind_speed;
    winfo.wave_offset = ::fmodf(winfo.wave_offset, 1.00f);

    auto& time_offset = ldata.time_offset;
    time_offset += ft.delta_millis() * ldata.wind_speed;
    time_offset = ::fmodf(time_offset, 1.00f);

    sp_.while_bound(logger, [&]() {
      sp_.set_uniform_vec4(logger, "u_clipPlane", ABOVE_VECTOR);
      sp_.set_uniform_vec3(logger, "u_camera_position", camera.world_position());
      sp_.set_uniform_float1(logger, "u_wave_offset", winfo.wave_offset);
      sp_.set_uniform_float1(logger, "u_wavestrength", ldata.wave_strength);
      sp_.set_uniform_float1(logger, "u_time_offset", time_offset);

      Material const water_material{};
      vao.while_bound(logger, [&]() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

        render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, tr, model_matrix, sp_, dinfo,
                                 water_material, registry, RECEIVES_AMBIENT_LIGHT,
                                 SET_NORMALMATRIX);

        glDisable(GL_BLEND);

        bind::global_unbind(logger, diffuse_);
        bind::global_unbind(logger, reflection_.tbo);
        bind::global_unbind(logger, reflection_.rbo.resource());
        bind::global_unbind(logger, refraction_.tbo);
        bind::global_unbind(logger, dudv_);
        bind::global_unbind(logger, normal_);
        bind::global_unbind(logger, refraction_.dbo);

        glActiveTexture(GL_TEXTURE0);
      });
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

ShaderProgram&
draw_water_options_to_shader(DrawWaterOptions const dwo, opengl::ShaderPrograms& sps)
{
  ShaderProgram* sp = nullptr;

  switch (dwo) {
  case DrawWaterOptions::None:
    std::abort();
    break;
  case DrawWaterOptions::Basic:
    sp = &sps.ref_sp("water_basic");
    break;
  case DrawWaterOptions::Medium:
    sp = &sps.ref_sp("water_medium");
    break;
  case DrawWaterOptions::Advanced:
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

  auto& logger   = es.logger;
  auto& zs       = lm.active();
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;

  Transform  transform;
  auto const render = [&](WaterInfo& winfo) {
    auto const& pos = winfo.position;

    auto& tr = transform.translation;
    tr.x     = pos.x;
    tr.z     = pos.y;

    // hack
    tr.y = 0.19999f; // pos.y;
    assert(tr.y < 2.0f);

    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;
    auto& vao   = dinfo.vao();

    auto const model_matrix = transform.model_matrix();

    winfo.wave_offset += ft.delta_millis() * ldata.wind_speed;
    winfo.wave_offset = ::fmodf(winfo.wave_offset, 1.00f);

    auto& time_offset = ldata.time_offset;
    time_offset += ft.delta_millis() * ldata.wind_speed;
    time_offset = ::fmodf(time_offset, 1.00f);

    sp_.while_bound(logger, [&]() {
      sp_.set_uniform_vec4(logger, "u_clipPlane", ABOVE_VECTOR);
      sp_.set_uniform_float1(logger, "u_time_offset", time_offset);

      Material const water_material{};
      vao.while_bound(logger, [&]() {
        glActiveTexture(GL_TEXTURE0);
        bind::global_bind(logger, diffuse_);

        glActiveTexture(GL_TEXTURE1);
        bind::global_bind(logger, normal_);

        render::draw_3dshape(rstate, GL_TRIANGLE_STRIP, model_matrix, sp_, dinfo);

        bind::global_unbind(logger, diffuse_);
        bind::global_unbind(logger, normal_);
        glActiveTexture(GL_TEXTURE0);
      });
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
  auto& ldata    = zs.level_data;

  Transform  transform;
  auto const render = [&](WaterInfo& winfo) {
    auto const& pos = winfo.position;

    auto& tr = transform.translation;
    tr.x     = pos.x;
    tr.z     = pos.y;

    // hack
    tr.y = 0.19999f; // pos.y;
    assert(tr.y < 2.0f);

    assert(winfo.dinfo);
    auto& dinfo = *winfo.dinfo;
    auto& vao   = dinfo.vao();

    bool constexpr RECEIVES_AMBIENT_LIGHT = true;
    bool constexpr SET_NORMALMATRIX       = false;
    auto const model_matrix               = transform.model_matrix();

    winfo.wave_offset += ft.delta_millis() * ldata.wind_speed;
    winfo.wave_offset = ::fmodf(winfo.wave_offset, 1.00f);

    auto& time_offset = ldata.time_offset;
    time_offset += ft.delta_millis() * ldata.wind_speed;
    time_offset = ::fmodf(time_offset, 1.00f);

    sp_.while_bound(logger, [&]() {
      sp_.set_uniform_vec4(logger, "u_clipPlane", ABOVE_VECTOR);
      sp_.set_uniform_float1(logger, "u_time_offset", time_offset);

      Material const water_material{};
      vao.while_bound(logger, [&]() {
        glActiveTexture(GL_TEXTURE0);
        bind::global_bind(logger, diffuse_);

        glActiveTexture(GL_TEXTURE1);
        bind::global_bind(logger, normal_);

        render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, tr, model_matrix, sp_, dinfo,
                                 water_material, registry, RECEIVES_AMBIENT_LIGHT,
                                 SET_NORMALMATRIX);

        bind::global_unbind(logger, diffuse_);
        bind::global_unbind(logger, normal_);
        glActiveTexture(GL_TEXTURE0);
      });
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

} // namespace boomhs
