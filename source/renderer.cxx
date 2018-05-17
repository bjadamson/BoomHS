#include <boomhs/billboard.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/types.hpp>
#include <boomhs/water.hpp>
#include <boomhs/water_fbos.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/factory.hpp>
#include <opengl/global.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

#include <extlibs/sdl.hpp>
#include <window/timer.hpp>

#include <iostream>
#include <stlw/log.hpp>
#include <stlw/math.hpp>
#include <stlw/random.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

glm::vec3 static constexpr VIEWING_OFFSET{0.5f, 0.0f, 0.5f};
auto static constexpr WIGGLE_UNDERATH_OFFSET = -0.2f;

namespace
{

void
enable_depth_tests()
{
  glEnable(GL_DEPTH_TEST);
}

void
disable_depth_tests()
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
}

struct CWState
{
  GLint winding;

  GLboolean culling_enabled;
  GLint     culling_mode;
};

CWState
read_cwstate()
{
  CWState cwstate;
  glGetIntegerv(GL_FRONT_FACE, &cwstate.winding);
  glGetBooleanv(GL_CULL_FACE, &cwstate.culling_enabled);
  glGetIntegerv(GL_CULL_FACE_MODE, &cwstate.culling_mode);
  return cwstate;
}

void
set_cwstate(CWState const& cw_state)
{
  glFrontFace(cw_state.winding);

  if (cw_state.culling_enabled) {
    glEnable(GL_CULL_FACE);
    glCullFace(cw_state.culling_mode);
  }
  else {
    glDisable(GL_CULL_FACE);
  }
}

void
set_modelmatrix(stlw::Logger& logger, glm::mat4 const& model_matrix, ShaderProgram& sp)
{
  sp.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
}

void
set_dirlight(stlw::Logger& logger, ShaderProgram& sp, GlobalLight const& global_light)
{
  auto const& directional_light = global_light.directional;
  sp.set_uniform_vec3(logger, "u_directional_light.direction", directional_light.direction);

  auto const& light = directional_light.light;
  sp.set_uniform_color_3fv(logger, "u_directional_light.diffuse", light.diffuse);
  sp.set_uniform_color_3fv(logger, "u_directional_light.specular", light.specular);

  sp.set_uniform_int1(logger, "u_ignore_dirlight", !directional_light.enabled);
}

void
set_pointlight(stlw::Logger& logger, ShaderProgram& sp, size_t const index,
               PointLight const& pointlight, glm::vec3 const& pointlight_position)
{
  std::string const varname = "u_pointlights[" + std::to_string(index) + "]";
  auto const make_field = [&varname](char const* fieldname) { return varname + "." + fieldname; };

  sp.set_uniform_color_3fv(logger, make_field("diffuse"), pointlight.light.diffuse);
  sp.set_uniform_color_3fv(logger, make_field("specular"), pointlight.light.specular);
  sp.set_uniform_vec3(logger, make_field("position"), pointlight_position);

  auto const& attenuation       = pointlight.attenuation;
  auto const  attenuation_field = [&make_field](char const* fieldname) {
    return make_field("attenuation.") + fieldname;
  };
  auto const constant  = attenuation_field("constant");
  auto const linear    = attenuation_field("linear");
  auto const quadratic = attenuation_field("quadratic");
  sp.set_uniform_float1(logger, constant, attenuation.constant);
  sp.set_uniform_float1(logger, linear, attenuation.linear);
  sp.set_uniform_float1(logger, quadratic, attenuation.quadratic);
}

struct PointlightTransform
{
  Transform const&  transform;
  PointLight const& pointlight;
};

void
set_receiveslight_uniforms(RenderState& rstate, glm::vec3 const& position,
                           glm::mat4 const& model_matrix, ShaderProgram& sp, DrawInfo& dinfo,
                           Material const&                         material,
                           std::vector<PointlightTransform> const& pointlights,
                           bool const                              receives_ambient_light)
{
  auto&       es    = rstate.es;
  auto&       zs    = rstate.zs;
  auto const& ldata = zs.level_data;

  auto&       logger       = es.logger;
  auto const& global_light = ldata.global_light;
  auto const& player       = ldata.player;

  set_modelmatrix(logger, model_matrix, sp);
  sp.set_uniform_matrix_3fv(logger, "u_normalmatrix",
                            glm::inverseTranspose(glm::mat3{model_matrix}));

  set_dirlight(logger, sp, global_light);

  // ambient
  if (receives_ambient_light) {
    LOG_INFO_SPRINTF("AMBIENT COLOR: %s", global_light.ambient.to_string());
    sp.set_uniform_color_3fv(logger, "u_ambient.color", global_light.ambient);
  }

  // specular
  sp.set_uniform_float1(logger, "u_reflectivity", 1.0f);

  // pointlight
  auto const view_matrix = rstate.view_matrix();
  {
    auto const inv_viewmatrix = glm::inverse(glm::mat3{view_matrix});
    sp.set_uniform_matrix_4fv(logger, "u_invviewmatrix", inv_viewmatrix);
  }

  FOR(i, pointlights.size())
  {
    auto const& transform  = pointlights[i].transform;
    auto const& pointlight = pointlights[i].pointlight;
    set_pointlight(logger, sp, i, pointlight, transform.translation);
  }

  // Material uniforms
  sp.set_uniform_vec3(logger, "u_material.ambient", material.ambient);
  sp.set_uniform_vec3(logger, "u_material.diffuse", material.diffuse);
  sp.set_uniform_vec3(logger, "u_material.specular", material.specular);
  sp.set_uniform_float1(logger, "u_material.shininess", material.shininess);
  // TODO: when re-implementing LOS restrictions
  // sp.set_uniform_vec3(logger, "u_player.position",  player.world_position());
  // sp.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
  // sp.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));

  // FOG uniforms
  sp.set_uniform_matrix_4fv(logger, "u_viewmatrix", view_matrix);

  auto const& fog = ldata.fog;
  sp.set_uniform_float1(logger, "u_fog.density", fog.density);
  sp.set_uniform_float1(logger, "u_fog.gradient", fog.gradient);
  sp.set_uniform_color(logger, "u_fog.color", fog.color);

  // misc
  sp.set_uniform_bool(logger, "u_drawnormals", es.draw_normals);
}

void
set_3dmvpmatrix(stlw::Logger& logger, glm::mat4 const& camera_matrix, glm::mat4 const& model_matrix,
                ShaderProgram& sp)
{
  auto const mvp_matrix = camera_matrix * model_matrix;
  sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
}

void
draw(RenderState& rstate, ShaderProgram& sp, DrawInfo& dinfo)
{
  auto&      es          = rstate.es;
  auto&      logger      = es.logger;
  auto const draw_mode   = es.wireframe_override ? GL_LINE_LOOP : dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET  = nullptr;

  /*
  LOG_DEBUG("---------------------------------------------------------------------------");
  LOG_DEBUG("drawing object!");
  LOG_DEBUG_SPRINTF("sp: %s", sp.to_string());
  LOG_DEBUG_SPRINTF("draw_info: %s", dinfo.to_string(sp.va()));
  LOG_DEBUG("---------------------------------------------------------------------------");
  */

  if (sp.instance_count) {
    auto const ic = *sp.instance_count;
    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, ic);
  }
  else {
    LOG_DEBUG_SPRINTF("Drawing %i indices", num_indices);
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
  }
}

void
draw_2d(RenderState& rstate, ShaderProgram& sp, DrawInfo& dinfo)
{
  disable_depth_tests();
  ON_SCOPE_EXIT([]() { enable_depth_tests(); });

  draw(rstate, sp, dinfo);
}

void
draw_2d(RenderState& rstate, ShaderProgram& sp, TextureInfo& ti, DrawInfo& dinfo)
{
  auto& logger = rstate.es.logger;
  ti.while_bound(logger, [&]() { draw_2d(rstate, sp, dinfo); });
}

void
draw_3dlit_shape(RenderState& rstate, glm::vec3 const& position, glm::mat4 const& model_matrix,
                 ShaderProgram& sp, DrawInfo& dinfo, Material const& material,
                 EntityRegistry& registry, bool const receives_ambient_light)
{
  auto& es     = rstate.es;
  auto& logger = es.logger;
  auto& zs     = rstate.zs;

  auto const                       pointlight_eids = find_pointlights(registry);
  std::vector<PointlightTransform> pointlights;

  FOR(i, pointlight_eids.size())
  {
    auto const&               eid        = pointlight_eids[i];
    auto&                     transform  = registry.get<Transform>(eid);
    auto&                     pointlight = registry.get<PointLight>(eid);
    PointlightTransform const plt{transform, pointlight};

    pointlights.emplace_back(plt);
  }
  set_receiveslight_uniforms(rstate, position, model_matrix, sp, dinfo, material, pointlights,
                             receives_ambient_light);
  auto const camera_matrix = rstate.camera_matrix();
  set_3dmvpmatrix(logger, camera_matrix, model_matrix, sp);

  draw(rstate, sp, dinfo);
}

void
draw_3dlightsource(RenderState& rstate, glm::mat4 const& model_matrix, ShaderProgram& sp,
                   DrawInfo& dinfo, EntityID const eid, EntityRegistry& registry)
{
  auto& es = rstate.es;
  auto& zs = rstate.zs;

  auto& logger     = es.logger;
  auto& pointlight = registry.get<PointLight>(eid);

  bool const has_texture = registry.has<TextureRenderable>(eid);
  if (!has_texture) {
    // ASSUMPTION: If the light source has a texture, then DO NOT set u_lightcolor.
    // Instead, assume the image should be rendered unaffected by the lightsource itself.
    auto const diffuse = pointlight.light.diffuse;
    sp.set_uniform_color_3fv(logger, "u_lightcolor", diffuse);
  }

  if (!sp.is_2d) {
    auto const& ldata         = zs.level_data;
    auto const  camera_matrix = rstate.camera_matrix();
    set_3dmvpmatrix(logger, camera_matrix, model_matrix, sp);
  }

  draw(rstate, sp, dinfo);
}

void
gl_log_callback(GLenum const source, GLenum const type, GLuint const id, GLenum const severity,
                GLsizei const length, GLchar const* message, void const* user_data)
{
  auto*       plogger = reinterpret_cast<stlw::Logger const*>(user_data);
  auto&       logger  = *const_cast<stlw::Logger*>(plogger);
  char const* prefix  = (type == GL_DEBUG_TYPE_ERROR) ? "** GL ERROR **" : "";
  LOG_ERROR_SPRINTF("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", prefix, type,
                    severity, message);

  std::abort();
}

void
billboard_spherical(float* data)
{
  // Column 0:
  data[0] = 1.0f;
  data[1] = 0.0f;
  data[2] = 0.0f;

  // Column 1:
  data[4 + 0] = 0.0f;
  data[4 + 1] = 1.0f;
  data[4 + 2] = 0.0f;

  // Column 2:
  data[8 + 0] = 0.0f;
  data[8 + 1] = 0.0f;
  data[8 + 2] = 1.0f;
}

void
billboard_cylindrical(float* data)
{
  // Column 0:
  data[0] = 1.0f;
  data[1] = 0.0f;
  data[2] = 0.0f;

  // Column 2:
  data[8 + 0] = 0.0f;
  data[8 + 1] = 0.0f;
  data[8 + 2] = 1.0f;
}

glm::mat4
compute_billboarded_viewmodel(Transform const& transform, glm::mat4 const& view_matrix,
                              BillboardType const bb_type)
{
  auto view_model = view_matrix * transform.model_matrix();

  // Reset the rotation values in order to achieve a billboard effect.
  //
  // http://www.geeks3d.com/20140807/billboarding-vertex-shader-glsl/
  float* data = glm::value_ptr(view_model);
  switch (bb_type) {
  case BillboardType::Spherical:
    billboard_spherical(data);
    break;
  case BillboardType::Cylindrical:
    billboard_cylindrical(data);
    break;
  default:
    std::abort();
  }

  auto const& s = transform.scale;
  data[0]       = s.x;
  data[5]       = s.y;
  data[10]      = s.z;

  return view_model;
}

} // namespace

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////
// RenderMatrices

RenderMatrices
RenderMatrices::from_camera_withposition(Camera const& camera, glm::vec3 const& custom_camera_pos)
{
  auto const  mode        = camera.mode();
  auto const& perspective = camera.perspective();
  auto const& ortho       = camera.ortho();

  auto const proj = Camera::compute_projectionmatrix(mode, perspective, ortho);

  auto const& target       = camera.get_target().translation;
  auto const  position_xyz = custom_camera_pos;
  auto const& up           = camera.eye_up();
  auto const& fps_center   = camera.world_forward() + target;

  auto const view = Camera::compute_viewmatrix(mode, position_xyz, target, up, fps_center);

  return RenderMatrices{proj, view};
}

RenderMatrices
RenderMatrices::from_camera(Camera const& camera)
{
  return from_camera_withposition(camera, camera.world_position());
}

///////////////////////////////////////////////////////////////////////////////////////////////
// RenderState
RenderState::RenderState(RenderMatrices const& rmatrices, EngineState& e, ZoneState& z)
    : rmatrices_(rmatrices)
    , es(e)
    , zs(z)
{
}

glm::mat4
RenderState::camera_matrix() const
{
  auto const proj = projection_matrix();
  auto const view = view_matrix();
  return proj * view;
}

glm::mat4
RenderState::projection_matrix() const
{
  return rmatrices_.projection;
}

glm::mat4
RenderState::view_matrix() const
{
  return rmatrices_.view;
}

} // namespace boomhs

namespace boomhs::render
{

void
init(stlw::Logger& logger, Dimensions const& dimensions)
{
  // Initialize opengl
  glViewport(0, 0, dimensions.w, dimensions.h);

  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // glDisable(GL_BLEND);

  enable_depth_tests();

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  // The logger is thread safe
  glDebugMessageCallback((GLDEBUGPROC)gl_log_callback, (void*)(&logger));
}

void
clear_screen(Color const& color)
{
  // https://stackoverflow.com/a/23944124/562174
  glDisable(GL_DEPTH_TEST);
  ON_SCOPE_EXIT([]() { enable_depth_tests(); });

  // Render
  glClearColor(color.r(), color.g(), color.b(), color.a());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
conditionally_draw_player_vectors(RenderState& rstate, WorldObject const& player)
{
  auto& es = rstate.es;
  auto& zs = rstate.zs;

  auto& logger = es.logger;

  glm::vec3 const pos = player.world_position();
  if (es.show_player_localspace_vectors) {
    // local-space
    //
    // forward
    auto const fwd = player.eye_forward();
    draw_arrow(rstate, pos, pos + fwd, LOC::GREEN);

    // right
    auto const right = player.eye_right();
    draw_arrow(rstate, pos, pos + right, LOC::RED);
  }
  if (es.show_player_worldspace_vectors) {
    // world-space
    //
    // forward
    auto const fwd = player.world_forward();
    draw_arrow(rstate, pos, pos + (2.0f * fwd), LOC::LIGHT_BLUE);

    // backward
    glm::vec3 const right = player.world_right();
    draw_arrow(rstate, pos, pos + right, LOC::PINK);
  }
}

void
draw_arrow(RenderState& rstate, glm::vec3 const& start, glm::vec3 const& head, Color const& color)
{
  auto& es = rstate.es;
  auto& zs = rstate.zs;

  auto& logger   = es.logger;
  auto& registry = zs.registry;

  auto& sps = zs.gfx_state.sps;
  auto& sp  = sps.ref_sp("3d_pos_color");

  auto        dinfo = OG::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});
  auto const& ldata = zs.level_data;

  Transform transform;
  sp.while_bound(logger, [&]() {
    auto const camera_matrix = rstate.camera_matrix();
    set_3dmvpmatrix(logger, camera_matrix, transform.model_matrix(), sp);

    dinfo.vao().while_bound(logger, [&]() { draw(rstate, sp, dinfo); });
  });
}

void
draw_arrow_abovetile_and_neighbors(RenderState& rstate, TilePosition const& tpos)
{
  auto&       es    = rstate.es;
  auto&       zs    = rstate.zs;
  auto const& ldata = zs.level_data;

  glm::vec3 constexpr offset{0.5f, 2.0f, 0.5f};

  auto const draw_the_arrow = [&](auto const& ntpos, auto const& color) {
    auto const bottom = glm::vec3{ntpos.x + offset.x, offset.y, ntpos.y + offset.y};
    auto const top    = bottom + (Y_UNIT_VECTOR * 2.0f);

    draw_arrow(rstate, top, bottom, color);
  };

  auto const& tgrid    = ldata.tilegrid();
  auto const neighbors = find_immediate_neighbors(tgrid, tpos, TileLookupBehavior::ALL_8_DIRECTIONS,
                                                  [](auto const& tpos) { return true; });
  assert(neighbors.size() <= 8);

  draw_the_arrow(tpos, LOC::BLUE);
  FOR(i, neighbors.size()) { draw_the_arrow(neighbors[i], LOC::LIME_GREEN); }
}

void
draw_global_axis(RenderState& rstate)
{
  auto& es  = rstate.es;
  auto& zs  = rstate.zs;
  auto& sps = zs.gfx_state.sps;

  auto& logger = es.logger;
  LOG_TRACE("Drawing Global Axis");

  auto& sp           = sps.ref_sp("3d_pos_color");
  auto  world_arrows = OG::create_axis_arrows(logger, sp);

  auto const& ldata = zs.level_data;
  Transform   transform;

  auto const draw_axis_arrow = [&](DrawInfo& dinfo) {
    auto& vao = dinfo.vao();
    vao.while_bound(logger, [&]() { draw(rstate, sp, dinfo); });
  };

  sp.while_bound(logger, [&]() {
    auto const camera_matrix = rstate.camera_matrix();
    set_3dmvpmatrix(logger, camera_matrix, transform.model_matrix(), sp);

    // assume for now they all share the same VAO layout
    draw_axis_arrow(world_arrows.x_dinfo);
    draw_axis_arrow(world_arrows.y_dinfo);
    draw_axis_arrow(world_arrows.z_dinfo);
  });

  LOG_TRACE("Finished Drawing Global Axis");
}

void
draw_local_axis(RenderState& rstate, glm::vec3 const& player_pos)
{
  auto& es  = rstate.es;
  auto& zs  = rstate.zs;
  auto& sps = zs.gfx_state.sps;

  auto& logger = es.logger;
  LOG_TRACE("Drawing Local Axis");

  auto& sp          = sps.ref_sp("3d_pos_color");
  auto  axis_arrows = OG::create_axis_arrows(logger, sp);

  Transform transform;
  transform.translation = player_pos;

  auto const& ldata = zs.level_data;

  sp.while_bound(logger, [&]() {
    auto const camera_matrix = rstate.camera_matrix();
    set_3dmvpmatrix(logger, camera_matrix, transform.model_matrix(), sp);

    // assume for now they all share the same VAO layout
    auto& vao = axis_arrows.x_dinfo.vao();
    vao.while_bound(logger, [&]() {
      draw(rstate, sp, axis_arrows.x_dinfo);
      draw(rstate, sp, axis_arrows.y_dinfo);
      draw(rstate, sp, axis_arrows.z_dinfo);
    });
  });

  LOG_TRACE("Finished Drawing Local Axis");
}

void
draw_entities(RenderState& rstate, stlw::float_generator& rng, FrameTime const& ft)
{
  auto const& es     = rstate.es;
  auto&       logger = es.logger;
  auto&       zs     = rstate.zs;

  assert(zs.gfx_state.gpu_state.entities);
  auto& entity_handles = *zs.gfx_state.gpu_state.entities;

  auto& registry = zs.registry;
  auto& sps      = zs.gfx_state.sps;

  auto const& ldata  = zs.level_data;
  auto const& player = ldata.player;

  auto const draw_fn = [&](auto eid, auto& sn, auto& transform, auto& is_visible, auto&&...) {
    bool const skip = !is_visible.value;
    if (skip) {
      return;
    }
    auto& dinfo = entity_handles.lookup(logger, eid);
    auto& sp    = sps.ref_sp(sn.value);
    auto& vao   = dinfo.vao();

    bool const is_lightsource = registry.has<PointLight>(eid);
    auto const model_matrix   = transform.model_matrix();

    sp.while_bound(logger, [&]() {
      vao.while_bound(logger, [&]() {
        if (is_lightsource) {
          assert(is_lightsource);
          draw_3dlightsource(rstate, model_matrix, sp, dinfo, eid, registry);
          return;
        }
        bool constexpr RECEIVES_AMBIENT_LIGHT = true;

        bool const receives_light = registry.has<Material>(eid);
        if (receives_light) {
          assert(registry.has<Material>(eid));
          Material const& material = registry.get<Material>(eid);
          draw_3dlit_shape(rstate, transform.translation, model_matrix, sp, dinfo, material,
                           registry, RECEIVES_AMBIENT_LIGHT);
          return;
        }

        // Can't receive light
        assert(!registry.has<Material>());

        if (!sp.is_2d) {
          auto const camera_matrix = rstate.camera_matrix();
          set_3dmvpmatrix(logger, camera_matrix, model_matrix, sp);
        }
        draw(rstate, sp, dinfo);
      });
    });
  };

  auto const draw_texture_fn = [&](auto const eid, auto& sn, auto& transform, auto& is_visible,
                                   auto& texture_renderable, auto&&... args) {
    auto* ti = texture_renderable.texture_info;
    assert(ti);
    ti->while_bound(logger, [&]() {
      draw_fn(eid, sn, transform, is_visible, texture_renderable, FORWARD(args));
    });
  };

  auto const draw_torch = [&](auto const eid, auto& sn, auto& transform, auto& isv, Torch& torch,
                              TextureRenderable& trenderable) {
    {
      auto& sp = sps.ref_sp(sn.value);

      // Describe glow
      static constexpr double MIN   = 0.3;
      static constexpr double MAX   = 1.0;
      static constexpr double SPEED = 0.135;
      auto const              a     = std::sin(ft.since_start_millis() * M_PI * SPEED);
      float const             glow  = glm::lerp(MIN, MAX, std::abs(a));
      sp.while_bound(logger, [&]() { sp.set_uniform_float1(logger, "u_glow", glow); });
    }

    // randomize the position slightly
    static constexpr auto DISPLACEMENT_MAX = 0.0015f;

    auto copy_transform = transform;
    copy_transform.translation.x += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.y += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.z += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);

    auto* ti = trenderable.texture_info;
    assert(ti);
    ti->while_bound(logger, [&]() { draw_fn(eid, sn, copy_transform, isv, torch); });
  };

  auto const draw_orbital_body = [&](auto const eid, auto& sn, auto& transform, auto& isv,
                                     auto& bboard, OrbitalBody&, TextureRenderable& trenderable) {
    auto const bb_type    = bboard.value;
    auto const view_model = compute_billboarded_viewmodel(transform, rstate.view_matrix(), bb_type);

    auto const proj_matrix = rstate.projection_matrix();
    auto const mvp_matrix  = proj_matrix * view_model;
    auto&      sp          = sps.ref_sp(sn.value);
    sp.while_bound(logger, [&]() { set_modelmatrix(logger, mvp_matrix, sp); });

    auto* ti = trenderable.texture_info;
    assert(ti);
    ti->while_bound(logger, [&]() { draw_fn(eid, sn, transform, isv, bboard); });
  };

#define COMMON ShaderName, Transform, IsVisible
  // define rendering order here
  // OrbitalBodies always render first.
  registry.view<COMMON, BillboardRenderable, OrbitalBody, TextureRenderable>().each(
      draw_orbital_body);

  registry.view<COMMON, WaterTileThing>().each(draw_fn);
  registry.view<COMMON, TextureRenderable, JunkEntityFromFILE>().each(draw_texture_fn);
  registry.view<COMMON, Color, JunkEntityFromFILE>().each(draw_fn);
  registry.view<COMMON, Torch, TextureRenderable>().each(draw_torch);
  registry.view<COMMON, CubeRenderable>().each(draw_fn);
  registry.view<COMMON, MeshRenderable, NPCData>().each(draw_fn);
  registry.view<COMMON, MeshRenderable, PlayerData>().each(draw_fn);
#undef COMMON
}

void
draw_fbo_testwindow(RenderState& rstate, glm::vec2 const& pos, glm::vec2 const& scale,
                    TextureInfo& ti)
{
  auto& es     = rstate.es;
  auto& logger = es.logger;

  auto& zs     = rstate.zs;
  auto& ttable = zs.gfx_state.texture_table;

  auto& sps = zs.gfx_state.sps;
  auto& sp  = sps.ref_sp("2dtexture");

  auto const v     = OF::rectangle_vertices();
  DrawInfo   dinfo = gpu::copy_rectangle_uvs(logger, GL_TRIANGLES, sp, v, ti);

  Transform transform;
  transform.translation = glm::vec3{pos.x, pos.y, 0.0f};
  transform.scale       = glm::vec3{scale.x, scale.y, 1.0};

  auto const model_matrix = transform.model_matrix();

  sp.while_bound(logger, [&]() {
    set_modelmatrix(logger, model_matrix, sp);

    auto& vao = dinfo.vao();
    vao.while_bound(logger, [&]() { draw_2d(rstate, sp, ti, dinfo); });
  });
}

void
draw_inventory_overlay(RenderState& rstate)
{
  auto& es     = rstate.es;
  auto& logger = es.logger;

  auto& zs  = rstate.zs;
  auto& sps = zs.gfx_state.sps;
  auto& sp  = sps.ref_sp("2dcolor");

  auto color = LOC::GRAY;
  color.set_a(0.25);
  OF::RectInfo const ri{1.0f, 1.0f, color, std::nullopt, std::nullopt};
  OF::RectBuffer     buffer = OF::make_rectangle(ri);

  DrawInfo dinfo = gpu::copy_rectangle(logger, GL_TRIANGLES, sp, buffer);

  auto& ttable = zs.gfx_state.texture_table;

  Transform  transform;
  auto const model_matrix = transform.model_matrix();

  sp.while_bound(logger, [&]() {
    set_modelmatrix(logger, model_matrix, sp);

    auto& vao = dinfo.vao();
    vao.while_bound(logger, [&]() { draw_2d(rstate, sp, dinfo); });
  });
}

void
draw_tilegrid(RenderState& rstate, TiledataState const& tilegrid_state, FrameTime const& ft)
{
  auto&       es    = rstate.es;
  auto&       zs    = rstate.zs;
  auto const& ldata = zs.level_data;

  auto& logger = es.logger;
  assert(zs.gfx_state.gpu_state.tiles);
  auto& tile_handles = *zs.gfx_state.gpu_state.tiles;
  auto& registry     = zs.registry;
  auto& sps          = zs.gfx_state.sps;

  auto const& tilegrid  = ldata.tilegrid();
  auto const& tiletable = ldata.tiletable();

  auto const& draw_tile_helper = [&](auto& sp, glm::vec3 const& position, DrawInfo& dinfo,
                                     Tile const& tile, glm::mat4 const& model_mat,
                                     bool const receives_ambient_light) {
    auto const& tileinfo = tiletable[tile.type];
    auto const& material = tileinfo.material;

    auto& vao = dinfo.vao();
    vao.while_bound(logger, [&]() {
      draw_3dlit_shape(rstate, position, model_mat, sp, dinfo, material, registry,
                       receives_ambient_light);
    });
  };
  auto const draw_tile = [&](auto const& tile_pos) {
    auto const& tile = tilegrid.data(tile_pos);
    if (!tilegrid_state.reveal && !tile.is_visible(registry)) {
      return;
    }
    auto const  tr               = tile_pos + VIEWING_OFFSET;
    auto&       transform        = registry.get<Transform>(tile.eid);
    auto const& rotation         = transform.rotation;
    auto const default_modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, transform.scale);
    auto&      dinfo             = tile_handles.lookup(logger, tile.type);

    switch (tile.type) {
    case TileType::FLOOR: {
      auto&      sp        = sps.ref_sp("floor");
      auto const scale     = glm::vec3{0.8};
      auto const modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, scale);
      sp.while_bound(logger, [&]() { draw_tile_helper(sp, tr, dinfo, tile, modmatrix, true); });
    } break;
    case TileType::WALL: {
      auto const inverse_model = glm::inverse(default_modmatrix);
      auto&      sp            = sps.ref_sp("hashtag");
      sp.while_bound(logger, [&]() {
        sp.set_uniform_matrix_4fv(logger, "u_inversemodelmatrix", inverse_model);
        draw_tile_helper(sp, tr, dinfo, tile, default_modmatrix, true);
      });
    } break;
    case TileType::RIVER:
      // Do nothing, we handle rendering rivers elsewhere.
      break;
    case TileType::STAIR_DOWN: {
      auto& sp = sps.ref_sp("stair");
      sp.while_bound(logger, [&]() {
        sp.set_uniform_color(logger, "u_color", LOC::WHITE);

        bool const receives_ambient_light = false;
        draw_tile_helper(sp, tr, dinfo, tile, default_modmatrix, receives_ambient_light);
      });
    } break;
    case TileType::STAIR_UP: {
      auto& sp = sps.ref_sp("stair");
      sp.while_bound(logger, [&]() {
        sp.set_uniform_color(logger, "u_color", LOC::WHITE);

        bool const receives_ambient_light = false;
        draw_tile_helper(sp, tr, dinfo, tile, default_modmatrix, receives_ambient_light);
      });
    } break;
    case TileType::BRIDGE:
    case TileType::DOOR:
    case TileType::TELEPORTER:
    default: {
      bool const receives_ambient_light = true;
      auto&      sp                     = sps.ref_sp("3d_pos_normal_color");
      sp.while_bound(logger, [&]() {
        draw_tile_helper(sp, tr, dinfo, tile, default_modmatrix, receives_ambient_light);
      });
    } break;
    case TileType::UNDEFINED:
      std::abort();
    }
  };
  visit_each(tilegrid, draw_tile);
}

void
draw_targetreticle(RenderState& rstate, window::FrameTime const& ft)
{
  auto&       es       = rstate.es;
  auto&       zs       = rstate.zs;
  auto&       registry = zs.registry;
  auto const& ldata    = zs.level_data;

  auto const& nearby_targets = ldata.nearby_targets;
  auto const  selected       = nearby_targets.selected();
  if (!selected) {
    return;
  }

  auto& sps = zs.gfx_state.sps;
  auto& sp  = sps.ref_sp("2dtexture");
  if (!sp.is_2d) {
    std::abort();
  }

  auto& logger = rstate.es.logger;
  auto& ttable = zs.gfx_state.texture_table;

  auto const selected_npc = *selected;
  assert(registry.has<Transform>(selected_npc));
  auto& npc_transform = registry.get<Transform>(selected_npc);

  Transform transform;
  transform.translation = npc_transform.translation;
  auto const scale      = nearby_targets.calculate_scale(ft);

  auto const      v = OF::rectangle_vertices();
  glm::mat4 const view_model =
      compute_billboarded_viewmodel(transform, rstate.view_matrix(), BillboardType::Spherical);

  auto const proj_matrix = rstate.projection_matrix();

  auto const draw_reticle = [&]() {
    auto constexpr ROTATE_SPEED = 80.0f;
    float const angle           = ROTATE_SPEED * ft.since_start_seconds();
    auto const  rot             = glm::angleAxis(glm::radians(angle), Z_UNIT_VECTOR);
    auto const  rmatrix         = glm::toMat4(rot);

    auto const mvp_matrix = proj_matrix * (view_model * rmatrix);
    set_modelmatrix(logger, mvp_matrix, sp);

    auto texture_o = ttable.find("TargetReticle");
    assert(texture_o);
    DrawInfo dinfo = gpu::copy_rectangle_uvs(logger, GL_TRIANGLES, sp, v, *texture_o);

    transform.scale = glm::vec3{scale};
    auto& vao       = dinfo.vao();
    vao.while_bound(logger, [&]() { draw_2d(rstate, sp, *texture_o, dinfo); });
  };

  auto const draw_glow = [&]() {
    auto texture_o = ttable.find("NearbyTargetGlow");
    assert(texture_o);

    auto const mvp_matrix = proj_matrix * view_model;
    set_modelmatrix(logger, mvp_matrix, sp);

    DrawInfo dinfo = gpu::copy_rectangle_uvs(logger, GL_TRIANGLES, sp, v, *texture_o);

    transform.scale = glm::vec3{scale};
    auto& vao       = dinfo.vao();
    vao.while_bound(logger, [&]() { draw_2d(rstate, sp, *texture_o, dinfo); });
  };

  sp.while_bound(logger, [&]() {
    if (scale < 1.0f) {
      draw_glow();
    }
    draw_reticle();
  });
}

void
draw_rivers(RenderState& rstate, window::FrameTime const& ft)
{
  auto& es     = rstate.es;
  auto& logger = es.logger;
  auto& zs     = rstate.zs;

  assert(zs.gfx_state.gpu_state.tiles);
  auto& tile_handles = *zs.gfx_state.gpu_state.tiles;
  auto& registry     = zs.registry;
  auto& sps          = zs.gfx_state.sps;

  auto& sp    = sps.ref_sp("river");
  auto& dinfo = tile_handles.lookup(logger, TileType::RIVER);

  sp.while_bound(logger, [&]() {
    sp.set_uniform_color(logger, "u_color", LOC::WHITE);

    auto const& level_data = zs.level_data;
    auto const& tile_info  = level_data.tiletable()[TileType::RIVER];
    auto const& material   = tile_info.material;

    auto const draw_river = [&](auto const& rinfo) {
      auto const& left  = rinfo.left;
      auto const& right = rinfo.right;

      auto const draw_wiggle = [&](auto const& wiggle) {
        sp.set_uniform_vec2(logger, "u_direction", wiggle.direction);
        sp.set_uniform_vec2(logger, "u_offset", wiggle.offset);

        auto const&     wp    = wiggle.position;
        auto const      tr    = glm::vec3{wp.x, WIGGLE_UNDERATH_OFFSET, wp.y} + VIEWING_OFFSET;
        glm::quat const rot   = glm::angleAxis(glm::degrees(rinfo.wiggle_rotation), Y_UNIT_VECTOR);
        auto const      scale = glm::vec3{0.5};

        bool const receives_ambient = true;
        auto const modelmatrix      = stlw::math::calculate_modelmatrix(tr, rot, scale);
        auto const inverse_model    = glm::inverse(modelmatrix);
        sp.set_uniform_matrix_4fv(logger, "u_inversemodelmatrix", inverse_model);
        draw_3dlit_shape(rstate, tr, modelmatrix, sp, dinfo, material, registry, receives_ambient);
      };
      for (auto const& w : rinfo.wiggles) {
        if (w.is_visible) {
          draw_wiggle(w);
        }
      }
    };
    auto const& rinfos = level_data.rivers();
    for (auto const& rinfo : rinfos) {
      draw_river(rinfo);
    }
  });
}

void
draw_skybox(RenderState& rstate, TextureInfo& tinfo, window::FrameTime const& ft)
{
  auto& zs       = rstate.zs;
  auto& registry = zs.registry;
  auto& sps      = zs.gfx_state.sps;

  auto const draw_fn = [&](auto const eid, auto& sn, auto& transform, IsVisible& is_visible,
                           IsSkybox&, TextureRenderable) {
    if (!is_visible.value) {
      return;
    }
    auto& entity_handles = *zs.gfx_state.gpu_state.entities;
    auto& es             = rstate.es;
    auto& logger         = es.logger;
    auto& dinfo          = entity_handles.lookup(logger, eid);

    // Can't receive light
    assert(!registry.has<Material>());

    auto& sp = sps.ref_sp(sn.value);
    LOG_TRACE_SPRINTF("drawing skybox with shader: %s", sn.value);

    auto const& ldata = zs.level_data;

    // Create a view matrix that has it's translation components zero'd out.
    //
    // The effect of this is the view matrix contains just the rotation, which is what's desired
    // for rendering the skybox.
    auto view_matrix  = rstate.view_matrix();
    view_matrix[3][0] = 0.0f;
    view_matrix[3][1] = 0.0f;
    view_matrix[3][2] = 0.0f;

    auto const proj_matrix   = rstate.projection_matrix();
    auto const camera_matrix = rstate.camera_matrix();
    auto const mvp_matrix    = camera_matrix * transform.model_matrix();

    sp.while_bound(logger, [&]() {
      sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);

      auto& vao = dinfo.vao();
      vao.while_bound(logger, [&]() { draw_2d(rstate, sp, tinfo, dinfo); });
    });
  };

  registry.view<ShaderName, Transform, IsVisible, IsSkybox, TextureRenderable>().each(draw_fn);
}

void
draw_stars(RenderState& rstate, window::FrameTime const& ft)
{
  auto& es     = rstate.es;
  auto& logger = es.logger;
  auto& zs     = rstate.zs;

  assert(zs.gfx_state.gpu_state.tiles);
  auto& tile_handles = *zs.gfx_state.gpu_state.tiles;
  auto& registry     = zs.registry;
  auto& sps          = zs.gfx_state.sps;

  auto const draw_starletter = [&](int const x, int const y, char const* shader,
                                   TileType const type) {
    auto& sp = sps.ref_sp(shader);
    sp.while_bound(logger, [&]() {
      sp.set_uniform_color_3fv(es.logger, "u_lightcolor", LOC::YELLOW);

      auto& dinfo = tile_handles.lookup(logger, type);

      auto constexpr Z    = 5.0f;
      auto const      tr  = glm::vec3{x, y, Z};
      glm::quat const rot = glm::angleAxis(glm::radians(90.0f), Z_UNIT_VECTOR);

      static constexpr double MIN   = 0.3;
      static constexpr double MAX   = 1.0;
      static constexpr double SPEED = 0.25;
      auto const              a     = std::sin(ft.since_start_seconds() * M_PI * SPEED);
      float const             scale = glm::lerp(MIN, MAX, std::abs(a));

      auto const scalevec     = glm::vec3{scale};
      auto const model_matrix = stlw::math::calculate_modelmatrix(tr, rot, scalevec);

      auto const& ldata         = zs.level_data;
      auto const  camera_matrix = rstate.camera_matrix();
      set_3dmvpmatrix(logger, camera_matrix, model_matrix, sp);

      auto& vao = dinfo.vao();
      vao.while_bound(logger, [&]() { draw(rstate, sp, dinfo); });
    });
  };

  auto constexpr X = -15.0;
  auto constexpr Y = 5.0;
  draw_starletter(X, Y, "light", TileType::STAR);
  draw_starletter(X, Y + 1, "light", TileType::BAR);
}

void
draw_terrain(RenderState& rstate, EntityRegistry& registry, FrameTime const& ft,
             glm::vec4 const& cull_plane)
{
  auto& es     = rstate.es;
  auto& logger = es.logger;

  auto&       zs      = rstate.zs;
  auto&       ldata   = zs.level_data;
  auto&       terrain = ldata.terrain();
  auto&       tgrid   = terrain.grid;
  auto const& trstate = terrain.render_state;

  // backup state to restore after drawing terrain
  auto const cw_state = read_cwstate();
  ON_SCOPE_EXIT([&]() { set_cwstate(cw_state); });

  Transform   transform;
  auto&       tr           = transform.translation;
  auto const& model_matrix = transform.model_matrix();
  Material    mat;

  bool constexpr ambient = true;
  auto const draw_piece  = [&](auto& t) {
    auto const& pos = t.position();
    tr.x            = pos.x;
    tr.z            = pos.y;

    auto const& config = t.config;
    glFrontFace(trstate.winding);
    if (trstate.culling_enabled) {
      glEnable(GL_CULL_FACE);
      glCullFace(trstate.culling_mode);
    }
    else {
      glDisable(GL_CULL_FACE);
    }

    // reach through the reference wrapper
    auto& sp = t.shader().get();
    sp.while_bound(logger, [&]() {
      sp.set_uniform_float1(logger, "u_uvmodifier", config.uv_modifier);
      sp.set_uniform_vec4(logger, "u_clipPlane", cull_plane);

      auto& dinfo = t.draw_info();
      auto& tinfo = t.texture_info();

      auto& vao = dinfo.vao();
      vao.while_bound(logger, [&]() {
        auto const draw_fn = [&]() {
          tinfo.set_fieldi(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
          tinfo.set_fieldi(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
          draw_3dlit_shape(rstate, tr, model_matrix, sp, dinfo, mat, registry, ambient);
        };
        tinfo.while_bound(logger, draw_fn);
      });
    });
  };

  LOG_TRACE("-------------------- Starting To Draw All Terrain(s) ----------------------");
  for (auto& t : tgrid) {
    draw_piece(t);
  }
  LOG_TRACE("-------------------------Finished Drawing All Terrain(s) ---------------------------");
}

void
draw_tilegrid(RenderState& rstate, TiledataState const& tds)
{
  auto& es = rstate.es;
  auto& zs = rstate.zs;

  auto& logger = es.logger;
  auto& sps    = zs.gfx_state.sps;
  auto& sp     = sps.ref_sp("3d_pos_color");

  auto const& leveldata = zs.level_data;
  auto const& tilegrid  = leveldata.tilegrid();

  Transform  transform;
  bool const show_y = tds.show_yaxis_lines;
  auto       dinfo  = OG::create_tilegrid(logger, sp, tilegrid, show_y);

  auto const model_matrix = transform.model_matrix();

  auto const& ldata = zs.level_data;

  sp.while_bound(logger, [&]() {
    auto const camera_matrix = rstate.camera_matrix();
    set_3dmvpmatrix(logger, camera_matrix, model_matrix, sp);

    auto& vao = dinfo.vao();
    vao.while_bound(logger, [&]() { draw(rstate, sp, dinfo); });
  });
}

void
draw_water(RenderState& rstate, EntityRegistry& registry, FrameTime const& ft,
           glm::vec4 const& cull_plane, WaterFrameBuffers& water_fbos,
           glm::vec3 const& camera_position)
{
  auto& es     = rstate.es;
  auto& logger = es.logger;

  auto& zs    = rstate.zs;
  auto& ldata = zs.level_data;

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
      sp.set_uniform_vec4(logger, "u_clipPlane", cull_plane);
      sp.set_uniform_vec3(logger, "u_camera_position", camera_position);
      sp.set_uniform_float1(logger, "u_wave_offset", winfo.wave_offset);
      sp.set_uniform_float1(logger, "u_wavestrength", ldata.wave_strength);
      sp.set_uniform_float1(logger, "u_time_offset", time_offset);

      vao.while_bound(logger, [&]() {
        water_fbos.while_bound(logger, [&]() {
          draw_3dlit_shape(rstate, tr, model_matrix, sp, dinfo, Material{}, registry,
                           RECEIVES_AMBIENT_LIGHT);
        });
      });
    });
  };

  LOG_TRACE("Rendering water");

  render(ldata.water());
  LOG_TRACE("Finished rendering water");
}

} // namespace boomhs::render
