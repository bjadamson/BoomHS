#include <boomhs/billboard.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/types.hpp>
#include <boomhs/water.hpp>

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
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void
disable_depth_tests()
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
}

void
set_modelmatrix(stlw::Logger& logger, glm::mat4 const& model_matrix, ShaderProgram& sp)
{
  sp.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
}

void
draw_drawinfo(stlw::Logger& logger, ShaderProgram& sp, DrawInfo const& dinfo)
{
  auto const draw_mode   = dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET  = nullptr;

  /*
  LOG_DEBUG("---------------------------------------------------------------------------");
  LOG_DEBUG("drawing object!");
  LOG_DEBUG("sp:\n" << sp << "");

  LOG_DEBUG("draw_info:");
  dinfo.print_self(logger, sp.va()));
  LOG_DEBUG("");
  LOG_DEBUG("---------------------------------------------------------------------------");
  */

  auto const draw_fn = [&]() {
    if (sp.instance_count) {
      auto const ic = *sp.instance_count;
      glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, ic);
    }
    else {
      glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
    }
  };

  if (dinfo.texture_info()) {
    auto const ti = *dinfo.texture_info();
    opengl::global::texture_bind(ti);
    ON_SCOPE_EXIT([&ti]() { opengl::global::texture_unbind(ti); });
    LOG_TRACE("texture info bound");
    draw_fn();
  }
  else {
    draw_fn();
  }
}

void
set_dirlight(stlw::Logger& logger, ShaderProgram& sp, GlobalLight const& global_light)
{
  auto const& directional_light = global_light.directional;
  sp.set_uniform_vec3(logger, "u_directional_light[0].direction", directional_light.direction);

  auto const& light = directional_light.light;
  sp.set_uniform_color_3fv(logger, "u_directional_light[0].diffuse", light.diffuse);
  sp.set_uniform_color_3fv(logger, "u_directional_light[0].specular", light.specular);

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
                           glm::mat4 const& model_matrix, ShaderProgram& sp, DrawInfo const& dinfo,
                           Material const&                         material,
                           std::vector<PointlightTransform> const& pointlights,
                           bool const                              receives_ambient_light)
{
  auto&       es    = rstate.es;
  auto&       zs    = rstate.zs;
  auto const& ldata = zs.level_data;

  auto&       logger       = es.logger;
  auto const& camera       = ldata.camera;
  auto const& global_light = ldata.global_light;
  auto const& player       = ldata.player;

  set_modelmatrix(logger, model_matrix, sp);
  sp.set_uniform_matrix_3fv(logger, "u_normalmatrix",
                            glm::inverseTranspose(glm::mat3{model_matrix}));

  set_dirlight(logger, sp, global_light);

  // ambient
  if (receives_ambient_light) {
    sp.set_uniform_color_3fv(logger, "u_ambient.color", global_light.ambient);
  }

  // specular
  sp.set_uniform_float1(logger, "u_reflectivity", 1.0f);

  // pointlight
  sp.set_uniform_matrix_4fv(logger, "u_invviewmatrix",
                            glm::inverse(glm::mat3{camera.view_matrix()}));

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
  sp.set_uniform_matrix_4fv(logger, "u_viewmatrix", camera.view_matrix());

  auto const& fog = ldata.fog;
  sp.set_uniform_float1(logger, "u_fog.density", fog.density);
  sp.set_uniform_float1(logger, "u_fog.gradient", fog.gradient);
  sp.set_uniform_color(logger, "u_fog.color", fog.color);

  // misc
  sp.set_uniform_bool(logger, "u_drawnormals", es.draw_normals);
}

void
draw(RenderState& rstate, glm::mat4 const& model_matrix, ShaderProgram& sp, DrawInfo const& dinfo,
     bool const is_skybox = false)
{
  auto& es     = rstate.es;
  auto& zs     = rstate.zs;
  auto& logger = es.logger;

  // Use the sp's PROGRAM and bind the sp's VAO.
  sp.use(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

  if (sp.is_2d) {
    disable_depth_tests();
    draw_drawinfo(logger, sp, dinfo);
    enable_depth_tests();
  }
  else {
    auto const& ldata  = zs.level_data;
    auto const& camera = ldata.camera;

    auto const mvp_matrix = camera.camera_matrix() * model_matrix;
    sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);

    if (is_skybox) {
      disable_depth_tests();
      draw_drawinfo(logger, sp, dinfo);
      enable_depth_tests();
    }
    else {
      draw_drawinfo(logger, sp, dinfo);
    }
  }
}

void
draw_3dlit_shape(RenderState& rstate, glm::vec3 const& position, glm::mat4 const& model_matrix,
                 ShaderProgram& sp, DrawInfo const& dinfo, Material const& material,
                 EntityRegistry& registry, bool const receives_ambient_light)
{
  auto& es = rstate.es;
  auto& zs = rstate.zs;

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
  draw(rstate, model_matrix, sp, dinfo);
}

void
draw_3dlightsource(RenderState& rstate, glm::mat4 const& model_matrix, ShaderProgram& sp,
                   DrawInfo const& dinfo, EntityID const eid, EntityRegistry& registry)
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
  draw(rstate, model_matrix, sp, dinfo);
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
compute_billboarded_viewmodel(Transform const& transform, Camera const& camera,
                              BillboardType const bb_type)
{
  auto view_model = camera.view_matrix() * transform.model_matrix();

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

namespace boomhs::render
{

void
init(stlw::Logger& logger, window::Dimensions const& dimensions)
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
  ON_SCOPE_EXIT([]() { glEnable(GL_DEPTH_TEST); });

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

  auto const dinfo = OG::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});

  Transform transform;
  draw(rstate, transform.model_matrix(), sp, dinfo);
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

  draw_the_arrow(tpos, LOC::BLUE);

  auto const& tgrid    = ldata.tilegrid();
  auto const neighbors = find_immediate_neighbors(tgrid, tpos, TileLookupBehavior::ALL_8_DIRECTIONS,
                                                  [](auto const& tpos) { return true; });
  assert(neighbors.size() <= 8);
  FOR(i, neighbors.size()) { draw_the_arrow(neighbors[i], LOC::LIME_GREEN); }
}

void
draw_global_axis(RenderState& rstate)
{
  auto& es  = rstate.es;
  auto& zs  = rstate.zs;
  auto& sps = zs.gfx_state.sps;

  auto& logger       = es.logger;
  auto& sp           = sps.ref_sp("3d_pos_color");
  auto  world_arrows = OG::create_axis_arrows(logger, sp);

  Transform  transform;
  auto const model_matrix = transform.model_matrix();
  draw(rstate, model_matrix, sp, world_arrows.x_dinfo);
  draw(rstate, model_matrix, sp, world_arrows.y_dinfo);
  draw(rstate, model_matrix, sp, world_arrows.z_dinfo);
}

void
draw_local_axis(RenderState& rstate, glm::vec3 const& player_pos)
{
  auto& es  = rstate.es;
  auto& zs  = rstate.zs;
  auto& sps = zs.gfx_state.sps;

  auto&      logger      = es.logger;
  auto&      sp          = sps.ref_sp("3d_pos_color");
  auto const axis_arrows = OG::create_axis_arrows(logger, sp);

  Transform transform;
  transform.translation = player_pos;

  auto const model_matrix = transform.model_matrix();
  draw(rstate, model_matrix, sp, axis_arrows.x_dinfo);
  draw(rstate, model_matrix, sp, axis_arrows.y_dinfo);
  draw(rstate, model_matrix, sp, axis_arrows.z_dinfo);
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
  auto const& camera = ldata.camera;
  auto const& player = ldata.player;

  auto const draw_fn = [&](auto eid, auto& sn, auto& transform, auto& is_visible, auto&&...) {
    bool const skip = !is_visible.value;
    if (skip) {
      return;
    }
    auto& sp    = sps.ref_sp(sn.value);
    auto& dinfo = entity_handles.lookup(logger, eid);

    bool const is_lightsource = registry.has<PointLight>(eid);
    auto const model_matrix   = transform.model_matrix();
    if (is_lightsource) {
      assert(is_lightsource);
      draw_3dlightsource(rstate, model_matrix, sp, dinfo, eid, registry);
      return;
    }
    bool constexpr receives_ambient_light = true;

    bool const receives_light = registry.has<Material>(eid);
    if (receives_light) {
      assert(registry.has<Material>(eid));
      Material const& material = registry.get<Material>(eid);
      draw_3dlit_shape(rstate, transform.translation, model_matrix, sp, dinfo, material, registry,
                       receives_ambient_light);
      return;
    }

    // Can't receive light
    assert(!registry.has<Material>());
    draw(rstate, transform.model_matrix(), sp, dinfo);
  };

  auto const player_drawfn = [&camera, &draw_fn](auto&&... args) {
    if (CameraMode::FPS == camera.mode()) {
      return;
    }
    draw_fn(FORWARD(args));
  };

  auto const draw_torch = [&](auto eid, auto& sn, auto& transform, auto&&... args) {
    {
      auto& sp = sps.ref_sp(sn.value);

      // Describe glow
      static constexpr double MIN   = 0.3;
      static constexpr double MAX   = 1.0;
      static constexpr double SPEED = 0.135;
      auto const              a     = std::sin(ft.since_start_millis() * M_PI * SPEED);
      float const             glow  = glm::lerp(MIN, MAX, std::abs(a));
      sp.set_uniform_float1(logger, "u_glow", glow);
    }

    // randomize the position slightly
    static constexpr auto DISPLACEMENT_MAX = 0.0015f;

    auto copy_transform = transform;
    copy_transform.translation.x += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.y += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.z += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);

    draw_fn(eid, sn, copy_transform, FORWARD(args));
  };

  auto const draw_orbital_body = [&](auto const eid, auto& sn, auto& transform, auto& isv,
                                     auto& bboard, auto&&... args) {
    auto& camera = zs.level_data.camera;

    auto const bb_type    = bboard.value;
    auto const view_model = compute_billboarded_viewmodel(transform, camera, bb_type);

    auto const mvp_matrix = camera.projection_matrix() * view_model;
    auto&      sp         = sps.ref_sp(sn.value);
    set_modelmatrix(logger, mvp_matrix, sp);

    draw_fn(eid, sn, transform, isv, bboard, FORWARD(args));
  };

#define COMMON ShaderName, Transform, IsVisible
  // define rendering order here
  // OrbitalBodies always render first.
  registry.view<COMMON, BillboardRenderable, OrbitalBody>().each(draw_orbital_body);

  registry.view<COMMON, IsTerrain>().each(draw_fn);
  registry.view<COMMON, Water>().each(draw_fn);
  registry.view<COMMON, JunkEntityFromFILE>().each(draw_fn);
  registry.view<COMMON, Torch>().each(draw_torch);
  registry.view<COMMON, MeshRenderable, NPCData>().each(draw_fn);
  registry.view<COMMON, MeshRenderable, PlayerData>().each(player_drawfn);
#undef COMMON
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

  auto const ti    = std::nullopt;
  DrawInfo   dinfo = gpu::copy_rectangle(logger, GL_TRIANGLES, sp, buffer, ti);

  Transform  transform;
  auto const model_matrix = transform.model_matrix();
  set_modelmatrix(logger, model_matrix, sp);

  draw(rstate, model_matrix, sp, dinfo);
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

  auto const& draw_tile_helper = [&](auto& sp, glm::vec3 const& position, auto const& dinfo,
                                     Tile const& tile, glm::mat4 const& model_mat,
                                     bool const receives_ambient_light) {
    auto const& tileinfo = tiletable[tile.type];
    auto const& material = tileinfo.material;

    draw_3dlit_shape(rstate, position, model_mat, sp, dinfo, material, registry,
                     receives_ambient_light);
  };
  auto const draw_tile = [&](auto const& tile_pos) {
    auto const& tile = tilegrid.data(tile_pos);
    if (!tilegrid_state.reveal && !tile.is_visible(registry)) {
      return;
    }
    // This offset causes the tile's to appear in the "middle"
    auto& tile_sp = sps.ref_sp("3d_pos_normal_color");

    auto const  tr               = tile_pos + VIEWING_OFFSET;
    auto&       transform        = registry.get<Transform>(tile.eid);
    auto const& rotation         = transform.rotation;
    auto const default_modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, transform.scale);
    auto const& dinfo            = tile_handles.lookup(logger, tile.type);

    switch (tile.type) {
    case TileType::FLOOR: {
      auto&      floor_sp  = sps.ref_sp("floor");
      auto const scale     = glm::vec3{0.8};
      auto const modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, scale);
      draw_tile_helper(floor_sp, tr, dinfo, tile, modmatrix, true);
    } break;
    case TileType::WALL: {
      auto const inverse_model = glm::inverse(default_modmatrix);
      auto&      sp            = sps.ref_sp("hashtag");
      sp.set_uniform_matrix_4fv(logger, "u_inversemodelmatrix", inverse_model);
      draw_tile_helper(sp, tr, dinfo, tile, default_modmatrix, true);
    } break;
    case TileType::RIVER:
      // Do nothing, we handle rendering rivers elsewhere.
      break;
    case TileType::STAIR_DOWN: {
      auto& sp = sps.ref_sp("stair");
      sp.set_uniform_color(logger, "u_color", LOC::WHITE);

      bool const receives_ambient_light = false;
      draw_tile_helper(sp, tr, dinfo, tile, default_modmatrix, receives_ambient_light);
    } break;
    case TileType::STAIR_UP: {
      auto& sp = sps.ref_sp("stair");
      sp.set_uniform_color(logger, "u_color", LOC::WHITE);

      bool const receives_ambient_light = false;
      draw_tile_helper(sp, tr, dinfo, tile, default_modmatrix, receives_ambient_light);
    } break;
    case TileType::BRIDGE:
    case TileType::DOOR:
    case TileType::TELEPORTER:
    default: {
      bool const receives_ambient_light = true;
      draw_tile_helper(tile_sp, tr, dinfo, tile, default_modmatrix, receives_ambient_light);
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
  auto& camera = ldata.camera;
  auto& ttable = zs.gfx_state.texture_table;

  auto const selected_npc = *selected;
  assert(registry.has<Transform>(selected_npc));
  auto& npc_transform = registry.get<Transform>(selected_npc);

  Transform transform;
  transform.translation = npc_transform.translation;
  auto const scale      = nearby_targets.calculate_scale(ft);

  auto const      v = OF::rectangle_vertices();
  glm::mat4 const view_model =
      compute_billboarded_viewmodel(transform, camera, BillboardType::Spherical);

  auto const draw_reticle = [&]() {
    glm::mat4 view_model =
        compute_billboarded_viewmodel(transform, camera, BillboardType::Spherical);

    auto constexpr ROTATE_SPEED = 80.0f;
    float const angle           = ROTATE_SPEED * ft.since_start_seconds();
    auto const  rot             = glm::angleAxis(glm::radians(angle), Z_UNIT_VECTOR);
    auto const  rmatrix         = glm::toMat4(rot);

    auto const mvp_matrix = camera.projection_matrix() * (view_model * rmatrix);
    set_modelmatrix(logger, mvp_matrix, sp);

    auto texture_o = ttable.find("TargetReticle");
    assert(texture_o);
    DrawInfo const di = gpu::copy_rectangle_uvs(logger, v, sp, *texture_o);

    transform.scale = glm::vec3{scale};
    draw(rstate, transform.model_matrix(), sp, di);
  };

  auto const draw_glow = [&]() {
    auto texture_o = ttable.find("NearbyTargetGlow");
    assert(texture_o);

    auto const mvp_matrix = camera.projection_matrix() * view_model;
    set_modelmatrix(logger, mvp_matrix, sp);

    DrawInfo const di = gpu::copy_rectangle_uvs(logger, v, sp, *texture_o);

    transform.scale = glm::vec3{scale / 2.0f};
    draw(rstate, transform.model_matrix(), sp, di);
  };

  if (scale < 1.0f) {
    draw_glow();
  }
  draw_reticle();
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

  auto&       sp    = sps.ref_sp("river");
  auto const& dinfo = tile_handles.lookup(logger, TileType::RIVER);

  sp.use(logger);
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
}

void
draw_skybox(RenderState& rstate, window::FrameTime const& ft)
{
  auto& zs       = rstate.zs;
  auto& registry = zs.registry;
  auto& sps      = zs.gfx_state.sps;

  auto const draw_fn = [&](auto const eid, auto& sn, auto& transform, IsVisible& is_visible,
                           IsSkybox&, auto&&...) {
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
    bool constexpr IS_SKYBOX = true;
    draw(rstate, transform.model_matrix(), sp, dinfo, IS_SKYBOX);
  };

  registry.view<ShaderName, Transform, IsVisible, IsSkybox>().each(draw_fn);
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
    sp.use(logger);
    sp.set_uniform_color_3fv(es.logger, "u_lightcolor", LOC::YELLOW);

    auto const& dinfo = tile_handles.lookup(logger, type);

    auto constexpr Z    = 5.0f;
    auto const      tr  = glm::vec3{x, y, Z};
    glm::quat const rot = glm::angleAxis(glm::radians(90.0f), Z_UNIT_VECTOR);

    static constexpr double MIN   = 0.3;
    static constexpr double MAX   = 1.0;
    static constexpr double SPEED = 0.25;
    auto const              a     = std::sin(ft.since_start_seconds() * M_PI * SPEED);
    float const             scale = glm::lerp(MIN, MAX, std::abs(a));

    auto const scalevec    = glm::vec3{scale};
    auto const modelmatrix = stlw::math::calculate_modelmatrix(tr, rot, scalevec);
    draw(rstate, modelmatrix, sp, dinfo);
  };

  auto constexpr X = -15.0;
  auto constexpr Y = 5.0;
  draw_starletter(X, Y, "light", TileType::STAR);
  draw_starletter(X, Y + 1, "light", TileType::BAR);
}

void
draw_terrain(RenderState& rstate, FrameTime const& ft)
{
  auto& zs  = rstate.zs;
  auto& sps = zs.gfx_state.sps;
  auto& sp  = sps.ref_sp("terrain");

  auto& es     = rstate.es;
  auto& logger = es.logger;
  sp.use(logger);

  auto&       ld           = zs.level_data;
  auto const& terrain      = ld.terrain();
  bool constexpr IS_SKYBOX = false;
  for (auto const& t : ld.terrain()) {
    Transform transform;

    auto const& pos         = t.position();
    transform.translation.x = pos.x;
    transform.translation.z = pos.y;

    draw(rstate, transform.model_matrix(), sp, t.draw_info(), IS_SKYBOX);
  }
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
  auto const dinfo  = OG::create_tilegrid(es.logger, sp, tilegrid, show_y);

  auto const model_matrix = transform.model_matrix();
  draw(rstate, model_matrix, sp, dinfo);
}

} // namespace boomhs::render
