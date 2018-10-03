#include <boomhs/billboard.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/components.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/fog.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/material.hpp>
#include <boomhs/math.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/screen_info.hpp>
#include <boomhs/zone_state.hpp>

#include <opengl/renderer.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/factory.hpp>
#include <opengl/global.hpp>
#include <opengl/gpu.hpp>
#include <opengl/light_renderer.hpp>
#include <opengl/shader.hpp>
#include <opengl/shapes.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/sdl.hpp>

#include <common/log.hpp>
#include <boomhs/math.hpp>
#include <boomhs/random.hpp>

using namespace boomhs;
using namespace boomhs::math::constants;
using namespace opengl;
using namespace gl_sdl;

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

void
set_fog(common::Logger& logger, Fog const& fog, glm::mat4 const& view_matrix, ShaderProgram& sp)
{
  sp.set_uniform_matrix_4fv(logger, "u_viewmatrix", view_matrix);

  sp.set_uniform_float1(logger, "u_fog.density", fog.density);
  sp.set_uniform_float1(logger, "u_fog.gradient", fog.gradient);
  sp.set_uniform_color(logger, "u_fog.color", fog.color);
}

void
gl_log_callback(GLenum const source, GLenum const type, GLuint const id, GLenum const severity,
                GLsizei const length, GLchar const* message, void const* user_data)
{
  auto*       plogger = reinterpret_cast<common::Logger const*>(user_data);
  auto&       logger  = *const_cast<common::Logger*>(plogger);
  char const* prefix  = (type == GL_DEBUG_TYPE_ERROR) ? "** GL ERROR **" : "";
  LOG_ERROR_SPRINTF("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", prefix, type,
                    severity, message);

  std::abort();
}

} // namespace

namespace opengl
{

///////////////////////////////////////////////////////////////////////////////////////////////
// DrawState
DrawState::DrawState()
    : DrawState(false)
{
}

DrawState::DrawState(bool const wireframe_override)
    : num_vertices(0)
    , num_drawcalls(0)
    , draw_wireframes(wireframe_override)
{
}

std::string
DrawState::to_string() const
{
  return fmt::sprintf("{vertices: %lu, drawcalls: %lu}", num_vertices, num_drawcalls);
}

} // namespace opengl

namespace opengl::render
{

CWState
read_cwstate()
{
  CWState cwstate;
  glGetIntegerv(GL_FRONT_FACE, &cwstate.winding.state);

  auto& culling = cwstate.culling;
  glGetBooleanv(GL_CULL_FACE, &culling.enabled);
  glGetIntegerv(GL_CULL_FACE_MODE, &culling.mode);
  return cwstate;
}

void
set_cwstate(CWState const& cw_state)
{
  glFrontFace(cw_state.winding.state);

  auto& culling = cw_state.culling;
  if (culling.enabled) {
    glEnable(GL_CULL_FACE);
    glCullFace(culling.mode);
  }
  else {
    glDisable(GL_CULL_FACE);
  }
}

BlendState
read_blendstate()
{
  BlendState bstate;
  bstate.enabled = glIsEnabled(GL_BLEND);

  glGetIntegerv(GL_BLEND_SRC_ALPHA, &bstate.source_alpha);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &bstate.dest_alpha);

  glGetIntegerv(GL_BLEND_SRC_RGB, &bstate.source_rgb);
  glGetIntegerv(GL_BLEND_DST_RGB, &bstate.dest_rgb);

  return bstate;
}

void
set_blendstate(BlendState const& state)
{
  glBlendFuncSeparate(state.source_rgb, state.dest_rgb, state.source_alpha, state.dest_alpha);

  if (state.enabled) {
    glEnable(GL_BLEND);
  }
  else {
    glDisable(GL_BLEND);
  }
}

void
init(common::Logger& logger)
{
  // Initialize opengl
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);

  enable_depth_tests();

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  // The logger is thread safe
  glDebugMessageCallback((GLDEBUGPROC)gl_log_callback, (void*)(&logger));

  int max_texunits_infragshader;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texunits_infragshader);
  LOG_TRACE_SPRINTF("GL_MAX_TEXTURE_IMAGE_UNITS (maximum number of texture units available in "
                    "fragment shader: %i",
                    max_texunits_infragshader);
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
draw_2d(RenderState& rstate, GLenum const dm, ShaderProgram& sp, DrawInfo& dinfo)
{
  disable_depth_tests();
  ON_SCOPE_EXIT([]() { enable_depth_tests(); });

  draw(rstate.fs.es.logger, rstate.ds, dm, sp, dinfo);
}

void
draw_2d(RenderState& rstate, GLenum const dm, ShaderProgram& sp, TextureInfo& ti, DrawInfo& dinfo)
{
  auto& fstate = rstate.fs;
  auto& logger = fstate.es.logger;
  ti.while_bound(logger, [&]() { draw_2d(rstate, dm, sp, dinfo); });
}

void
draw_3dlightsource(RenderState& rstate, GLenum const dm, glm::mat4 const& model_matrix,
                   ShaderProgram& sp, DrawInfo& dinfo, EntityID const eid, EntityRegistry& registry)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& zs     = fstate.zs;

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
    auto const  camera_matrix = fstate.camera_matrix();
    set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
  }

  draw(logger, rstate.ds, dm, sp, dinfo);
}

void
draw_3dshape(RenderState& rstate, GLenum const dm, glm::mat4 const& model_matrix, ShaderProgram& sp,
             DrawInfo& dinfo)
{
  auto& fstate = rstate.fs;

  auto& es     = fstate.es;
  auto& logger = es.logger;

  set_modelmatrix(logger, model_matrix, sp);

  auto const camera_matrix = fstate.camera_matrix();
  set_mvpmatrix(logger, camera_matrix, model_matrix, sp);

  {
    auto const& zs          = fstate.zs;
    auto const& ldata       = zs.level_data;
    auto const& fog         = ldata.fog;
    auto const  view_matrix = fstate.view_matrix();
    set_fog(logger, fog, view_matrix, sp);
  }

  // misc
  sp.set_uniform_bool(logger, "u_drawnormals", es.draw_normals);
  draw(logger, rstate.ds, dm, sp, dinfo);
}

void
draw_3dblack_water(RenderState& rstate, GLenum const dm, glm::mat4 const& model_matrix,
                   ShaderProgram& sp, DrawInfo& dinfo)
{
  auto& fstate = rstate.fs;

  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto const camera_matrix = fstate.camera_matrix();
  set_mvpmatrix(logger, camera_matrix, model_matrix, sp);

  draw(logger, rstate.ds, dm, sp, dinfo);
}

void
draw_3dlit_shape(RenderState& rstate, GLenum const dm, glm::vec3 const& position,
                 glm::mat4 const& model_matrix, ShaderProgram& sp, DrawInfo& dinfo,
                 Material const& material, EntityRegistry& registry, bool const set_normalmatrix)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;
  auto& zs     = fstate.zs;

  if (!es.draw_normals) {
    LightRenderer::set_light_uniforms(rstate, registry, sp, material, position, model_matrix, set_normalmatrix);
  }

  draw_3dshape(rstate, dm, model_matrix, sp, dinfo);
}

void
draw(common::Logger& logger, DrawState& ds, GLenum const dm, ShaderProgram& sp, DrawInfo& dinfo)
{
  auto const draw_mode   = ds.draw_wireframes ? GL_LINE_LOOP : dm;
  auto const num_indices = dinfo.num_indices();

  /*
  LOG_DEBUG("---------------------------------------------------------------------------");
  LOG_DEBUG("drawing object!");
  LOG_DEBUG_SPRINTF("sp: %s", sp.to_string());
  LOG_DEBUG_SPRINTF("draw_info: %s", dinfo.to_string(sp.va()));
  LOG_DEBUG("---------------------------------------------------------------------------");
  */

  FOR_DEBUG_ONLY([&]() { assert(sp.is_bound()); });
  FOR_DEBUG_ONLY([&]() { assert(dinfo.is_bound()); });
  draw_elements(logger, draw_mode, sp, num_indices);

  ds.num_vertices += num_indices;
  ++ds.num_drawcalls;
}

void
draw_elements(common::Logger& logger, GLenum const draw_mode, ShaderProgram& sp,
              GLuint const num_indices)
{
  auto constexpr INDICES_PTR = nullptr;

  if (sp.instance_count) {
    auto const prim_count = *sp.instance_count;
    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, INDICES_PTR, prim_count);
  }
  else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, INDICES_PTR);
  }
}

void
draw_2delements(common::Logger& logger, GLenum const draw_mode, ShaderProgram& sp,
                GLuint const num_indices)
{
  disable_depth_tests();
  ON_SCOPE_EXIT([]() { enable_depth_tests(); });

  draw_elements(logger, draw_mode, sp, num_indices);
}

void
draw_2delements(common::Logger& logger, GLenum const draw_mode, ShaderProgram& sp, TextureInfo& ti,
                GLuint const num_indices)
{
  BIND_UNTIL_END_OF_SCOPE(logger, ti);
  draw_2delements(logger, draw_mode, sp, ti, num_indices);
}

void
draw_arrow(RenderState& rstate, glm::vec3 const& start, glm::vec3 const& head, Color const& color)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& zs     = fstate.zs;

  auto& logger   = es.logger;
  auto& registry = zs.registry;

  auto& sps = zs.gfx_state.sps;
  auto& sp  = sps.ref_sp("3d_pos_color");

  auto const acp = ArrowCreateParams{color, start, head};
  auto const arrow_v = ArrowFactory::create_vertices(acp);
  auto        dinfo = OG::copy_arrow(logger, sp.va(), arrow_v);

  Transform transform;
  sp.while_bound(logger, [&]() {
    auto const camera_matrix = fstate.camera_matrix();
    set_mvpmatrix(logger, camera_matrix, transform.model_matrix(), sp);

    dinfo.while_bound(logger, [&]() { draw(logger, rstate.ds, GL_LINES, sp, dinfo); });
  });
}

void
draw_line(RenderState& rstate, glm::vec3 const& start, glm::vec3 const& end, Color const& color)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& zs     = fstate.zs;

  auto& logger   = es.logger;
  auto& registry = zs.registry;

  auto& sps = zs.gfx_state.sps;
  auto& sp  = sps.ref_sp("line");

  auto const  lcp   = LineCreateParams{start, end};
  auto const  line_v = LineFactory::create_vertices(lcp);
  auto        dinfo = OG::copy_line(logger, sp.va(), line_v);

  sp.while_bound(logger, [&]() {
    sp.set_uniform_color(logger, "u_linecolor", color);
    dinfo.while_bound(logger, [&]() { draw_2d(rstate, GL_LINES, sp, dinfo); });
  });
}

void
draw_fbo_testwindow(RenderState& rstate, glm::vec2 const& pos, glm::vec2 const& size,
                    ShaderProgram& sp, TextureInfo& ti)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto const v     = OF::rectangle_vertices(pos.x, pos.y, size.x, size.y);
  auto const uv = OF::rectangle_uvs(ti.uv_max);
  auto const vuvs  = RectangleFactory::from_vertices_and_uvs(v, uv);

  DrawInfo   dinfo = gpu::copy_rectangle_uvs(logger, sp.va(), vuvs);

  auto const proj_matrix = fstate.projection_matrix();
  sp.while_bound(logger, [&]() {
    sp.set_uniform_matrix_4fv(logger, "u_projmatrix", proj_matrix);

    glActiveTexture(GL_TEXTURE0);

    dinfo.while_bound(logger, [&]() { draw_2d(rstate, GL_TRIANGLES, sp, ti, dinfo); });

    glActiveTexture(GL_TEXTURE0);
  });
}

void
draw_targetreticle(RenderState& rstate, FrameTime const& ft)
{
  auto&       fstate   = rstate.fs;
  auto&       es       = fstate.es;
  auto&       zs       = fstate.zs;
  auto&       registry = zs.registry;
  auto const& ldata    = zs.level_data;

  auto const& nearby_targets = ldata.nearby_targets;
  auto const  selected       = nearby_targets.selected();
  if (!selected) {
    return;
  }

  auto& logger = fstate.es.logger;
  auto& ttable = zs.gfx_state.texture_table;

  auto const npc_selected_eid = *selected;
  assert(registry.has<Transform>(npc_selected_eid));
  auto& npc_transform = registry.get<Transform>(npc_selected_eid);

  Transform transform;
  transform.translation = npc_transform.translation;
  auto const scale      = nearby_targets.calculate_scale(ft);
  transform.scale = glm::vec3{scale};

  auto const proj_matrix = fstate.projection_matrix();

  auto const draw_billboard = [](RenderState& rstate, Transform& transform, ShaderProgram& sp,
                                 char const* texture_name) {
    auto& fstate = rstate.fs;
    auto& es     = fstate.es;
    auto& zs     = fstate.zs;

    auto& gfx_state = zs.gfx_state;
    auto& ttable = gfx_state.texture_table;
    auto& sps    = gfx_state.sps;

    auto& logger = es.logger;

    auto texture_o = ttable.find(texture_name);
    assert(texture_o);
    auto& ti = *texture_o;

    auto const      v = OF::rectangle_vertices_default();
    auto const uv = OF::rectangle_uvs(ti.uv_max);
    auto const vuvs  = RectangleFactory::from_vertices_and_uvs(v, uv);
    DrawInfo dinfo = gpu::copy_rectangle_uvs(logger, sp.va(), vuvs);

    dinfo.while_bound(logger, [&]() { draw_2d(rstate, GL_TRIANGLES, sp, ti, dinfo); });
  };

  glm::mat4 const view_model =
      Billboard::compute_viewmodel(transform, fstate.view_matrix(), BillboardType::Spherical);
  auto const draw_reticle = [&](auto& sp) {
    auto constexpr ROTATE_SPEED = 80.0f;
    float const angle           = ROTATE_SPEED * ft.since_start_seconds();
    auto const  rot             = glm::angleAxis(glm::radians(angle), Z_UNIT_VECTOR);
    auto const  rmatrix         = glm::toMat4(rot);

    auto const mvp_matrix = proj_matrix * (view_model * rmatrix);
    sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);

    auto const& player = find_player(registry);
    auto const target_level = registry.get<NPCData>(npc_selected_eid).level;
    auto const blendc = NearbyTargets::color_from_level_difference(player.level, target_level);
    sp.set_uniform_color(logger, "u_blendcolor", blendc);

    draw_billboard(rstate, transform, sp, "TargetReticle");
  };

  auto const draw_glow = [&](auto& sp) {
    auto const mvp_matrix = proj_matrix * view_model;
    sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
    draw_billboard(rstate, transform, sp, "NearbyTargetGlow");
  };

  bool const should_draw_glow = scale < 1.0f;

  auto& sps = zs.gfx_state.sps;

  auto& sp = sps.ref_sp(should_draw_glow ? "billboard" : "target_reticle");
  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  ENABLE_ALPHA_BLENDING_UNTIL_SCOPE_EXIT();
  if (should_draw_glow) {
    draw_glow(sp);
  }
  else {
    draw_reticle(sp);
  }
}

void
draw_grid_lines(RenderState& rstate)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& zs     = fstate.zs;

  auto& logger = es.logger;
  auto& sps    = zs.gfx_state.sps;
  auto& sp     = sps.ref_sp("3d_pos_color");

  bool const show_y = es.show_yaxis_lines;

  glm::vec2 constexpr GRID_DIMENSIONS{20, 20};

  auto const draw_the_terrain_grid = [&](glm::mat4 const& model_matrix, auto const& color) {
    auto const grid = GridFactory::create_grid(GRID_DIMENSIONS, show_y, color);
    auto dinfo = OG::copy_grid_gpu(logger, sp.va(), grid);

    sp.while_bound(logger, [&]() {
      auto const camera_matrix = fstate.camera_matrix();
      set_mvpmatrix(logger, camera_matrix, model_matrix, sp);

      dinfo.while_bound(logger, [&]() { draw(logger, rstate.ds, GL_LINES, sp, dinfo); });
    });
  };


  Transform transform;
  draw_the_terrain_grid(transform.model_matrix(), LOC::RED);

  transform.translation = glm::vec3{-GRID_DIMENSIONS.x, 0.0f, 0};
  draw_the_terrain_grid(transform.model_matrix(), LOC::BLUE);

  transform.translation = glm::vec3{-GRID_DIMENSIONS.x, 0.0f, -GRID_DIMENSIONS.y};
  draw_the_terrain_grid(transform.model_matrix(), LOC::GREEN);

  transform.translation = glm::vec3{-GRID_DIMENSIONS.x, 0.0f, GRID_DIMENSIONS.y};
  draw_the_terrain_grid(transform.model_matrix(), LOC::ORANGE);

  transform.translation = glm::vec3{GRID_DIMENSIONS.x, 0.0f, 0};
  draw_the_terrain_grid(transform.model_matrix(), LOC::PURPLE);

  transform.translation = glm::vec3{GRID_DIMENSIONS.x, 0.0f, -GRID_DIMENSIONS.y};
  draw_the_terrain_grid(transform.model_matrix(), LOC::BROWN);

  transform.translation = glm::vec3{GRID_DIMENSIONS.x, 0.0f, GRID_DIMENSIONS.y};
  draw_the_terrain_grid(transform.model_matrix(), LOC::NAVY);

  transform.translation = glm::vec3{-GRID_DIMENSIONS.x, 0.0f, -GRID_DIMENSIONS.y};
  draw_the_terrain_grid(transform.model_matrix(), LOC::YELLOW);

  transform.translation = glm::vec3{GRID_DIMENSIONS.x, 0.0f, GRID_DIMENSIONS.y};
  draw_the_terrain_grid(transform.model_matrix(), LOC::GRAY);
}

void
set_modelmatrix(common::Logger& logger, glm::mat4 const& model_matrix, ShaderProgram& sp)
{
  sp.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
}

void
set_mvpmatrix(common::Logger& logger, glm::mat4 const& camera_matrix, glm::mat4 const& model_matrix,
              ShaderProgram& sp)
{
  auto const mvp_matrix = camera_matrix * model_matrix;
  sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
}

namespace detail
{

// Handles extracting the correct values out of the Viewport for the function.
//
// Primary purpose is to handle conversion from
//   left, top, width, height
//
// to
//   left, bottom, width, height
void
gl_fn_with_viewport(Viewport const& vp, void(*fn)(int, int, int, int))
{
  auto const left   = vp.left();

  // TODO:
  // vp.bottom() for TEST program. vp.top() for boomhs program. WHY?
  auto const bottom = vp.top();


  auto const width  = vp.width();
  auto const height = vp.height();

  fn(left, bottom, width, height);
}

} // namespace detail

void
set_viewport(Viewport const& vp)
{
  detail::gl_fn_with_viewport(vp, glViewport);
}

void
set_scissor(Viewport const& vp)
{
  detail::gl_fn_with_viewport(vp, glScissor);
}

void
set_viewport_and_scissor(Viewport const& vp)
{
  set_viewport(vp);
  set_scissor(vp);
}

} // namespace opengl::render
