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

#include <stlw/random.hpp>

using namespace opengl;
using namespace window;

float constexpr CUTOFF_HEIGHT      = 0.4f;
glm::vec4 constexpr ABOVE_VECTOR   = {0, -1, 0, CUTOFF_HEIGHT};
glm::vec4 constexpr BENEATH_VECTOR = {0, 1, 0, -CUTOFF_HEIGHT};

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// WaterInfo
WaterInfo::WaterInfo(glm::vec2 const& p, DrawInfo&& d, ShaderProgram& s, TextureInfo& t)
    : position(p)
    , dinfo(MOVE(d))
    , shader(s)
    , tinfo(&t)
{
}

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
WaterFactory::generate_info(stlw::Logger& logger, WaterInfoConfig const& wic, ShaderProgram& sp,
                            TextureInfo& tinfo)
{
  auto const data = generate_water_data(logger, wic.dimensions, wic.num_vertexes);
  LOG_TRACE_SPRINTF("Generated water piece: %s", data.to_string());

  BufferFlags const flags{true, false, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, sp.va(), buffer);

  return WaterInfo{wic.position, MOVE(di), sp, tinfo};
}

WaterInfo
WaterFactory::make_default(stlw::Logger& logger, ShaderPrograms& sps, TextureTable& ttable)
{
  LOG_TRACE("Generating water");
  glm::vec2 const       pos{0, 0};
  size_t const          num_vertexes = 64;
  glm::vec2 const       dimensions{20};
  WaterInfoConfig const wic{pos, dimensions, num_vertexes};

  auto texture_o = ttable.find("water-diffuse");
  assert(texture_o);
  auto& ti = *texture_o;

  // These uniforms only need to be set once.
  auto& sp = sps.ref_sp("water");

  auto wi = generate_info(logger, wic, sp, ti);

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
// WaterRenderer
WaterRenderer::WaterRenderer(WaterFrameBuffers&& wfb)
    : fbos_(MOVE(wfb))
{
}

void
WaterRenderer::render_reflection(EngineState& es, DrawState& ds, LevelManager& lm, Camera& camera,
                                 SkyboxRenderer& skybox_renderer, stlw::float_generator& rng,
                                 FrameTime const& ft)
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

  fbos_.with_reflection_fbo(logger, [&]() {
    render::clear_screen(fog_color);
    skybox_renderer.render(rstate, ds, ft);
    render::render_scene(rstate, lm, rng, ft, ABOVE_VECTOR);
  });
}

void
WaterRenderer::render_refraction(EngineState& es, DrawState& ds, LevelManager& lm, Camera& camera,
                                 SkyboxRenderer& skybox_renderer, stlw::float_generator& rng,
                                 FrameTime const& ft)
{
  auto&       zs        = lm.active();
  auto&       logger    = es.logger;
  auto&       ldata     = zs.level_data;
  auto const& fog_color = ldata.fog.color;

  auto const  fmatrices = FrameMatrices::from_camera(camera);
  FrameState  fstate{fmatrices, es, zs};
  RenderState rstate{fstate, ds};

  fbos_.with_refraction_fbo(logger, [&]() {
    render::clear_screen(fog_color);

    skybox_renderer.render(rstate, ds, ft);
    render::render_scene(rstate, lm, rng, ft, BENEATH_VECTOR);
  });
}

void
WaterRenderer::render_water(RenderState& rstate, DrawState& ds, LevelManager& lm, Camera& camera,
                            FrameTime const& ft)
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

    auto& sp    = winfo.shader;
    auto& dinfo = winfo.dinfo;
    auto& vao   = dinfo.vao();

    bool constexpr RECEIVES_AMBIENT_LIGHT = true;
    auto const model_matrix               = transform.model_matrix();

    winfo.wave_offset += ft.delta_millis() * ldata.wind_speed;
    winfo.wave_offset = ::fmodf(winfo.wave_offset, 1.00f);

    auto& time_offset = ldata.time_offset;
    time_offset += ft.delta_millis() * ldata.wind_speed;
    time_offset = ::fmodf(time_offset, 1.00f);

    sp.while_bound(logger, [&]() {
      sp.set_uniform_vec4(logger, "u_clipPlane", ABOVE_VECTOR);
      sp.set_uniform_vec3(logger, "u_camera_position", camera.world_position());
      sp.set_uniform_float1(logger, "u_wave_offset", winfo.wave_offset);
      sp.set_uniform_float1(logger, "u_wavestrength", ldata.wave_strength);
      sp.set_uniform_float1(logger, "u_time_offset", time_offset);

      Material const water_material{};
      vao.while_bound(logger, [&]() {
        fbos_.while_bound(logger, [&]() {
          render::draw_3dlit_shape(rstate, GL_TRIANGLE_STRIP, tr, model_matrix, sp, dinfo,
                                   water_material, registry, RECEIVES_AMBIENT_LIGHT);
        });
      });
    });
  };

  LOG_TRACE("Rendering water");
  render(ldata.water);
  LOG_TRACE("Finished rendering water");
}

} // namespace boomhs
