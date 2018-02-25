#include <boomhs/renderer.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/types.hpp>

#include <opengl/factory.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/global.hpp>
#include <opengl/gpu.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>

#include <window/timer.hpp>

#include <stlw/math.hpp>
#include <stlw/random.hpp>
#include <stlw/log.hpp>
#include <iostream>

using namespace boomhs;
using namespace opengl;
using namespace window;

glm::vec3 static constexpr VIEWING_OFFSET{0.0f, 0.0f, 0.0f};
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
set_modelmatrix(stlw::Logger &logger, glm::mat4 const& model_matrix, ShaderProgram &sp)
{
  sp.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
}

void
set_mvpmatrix(stlw::Logger &logger, glm::mat4 const& model_matrix, ShaderProgram &sp,
    Camera const& camera)
{
  glm::mat4 const cam_matrix = camera.camera_matrix();
  auto const mvp_matrix = cam_matrix * model_matrix;

  sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
}

void
draw_drawinfo(stlw::Logger &logger, ShaderProgram &sp, DrawInfo const& dinfo)
{
  auto const draw_mode = dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET = nullptr;

  /*
  std::cerr << "---------------------------------------------------------------------------\n";
  std::cerr << "drawing object!\n";
  std::cerr << "sp:\n" << sp << "\n";

  std::cerr << "draw_info:\n";
  dinfo.print_self(std::cerr, sp.va());
  std::cerr << "\n";
  std::cerr << "---------------------------------------------------------------------------\n";
  */

  auto const draw_fn = [&]() {
    if (sp.instance_count) {
      auto const ic = *sp.instance_count;
      glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, ic);
    } else {
      glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
    }
  };

  if (dinfo.texture_info()) {
    auto const ti = *dinfo.texture_info();
    opengl::global::texture_bind(ti);
    ON_SCOPE_EXIT([&ti]() { opengl::global::texture_unbind(ti); });
    draw_fn();
  } else {
    draw_fn();
  }
}

void
set_dirlight(stlw::Logger &logger, ShaderProgram &sp, GlobalLight const& global_light)
{
  auto const& directional_light = global_light.directional;
  sp.set_uniform_vec3(logger, "u_dirlight.direction", directional_light.direction);

  auto const& light = directional_light.light;
  sp.set_uniform_color_3fv(logger, "u_dirlight.diffuse", light.diffuse);
  sp.set_uniform_color_3fv(logger, "u_dirlight.specular", light.specular);
}

void
set_pointlight(stlw::Logger &logger, ShaderProgram &sp, size_t const index,
    PointLight const& pointlight, glm::vec3 const& pointlight_position)
{
  std::string const varname = "u_pointlights[" + std::to_string(index) + "]";
  auto const make_field = [&varname](char const* fieldname) {
    return varname + "." + fieldname;
  };

  sp.set_uniform_color_3fv(logger, make_field("diffuse"), pointlight.light.diffuse);
  sp.set_uniform_color_3fv(logger, make_field("specular"), pointlight.light.specular);
  sp.set_uniform_vec3(logger, make_field("position"), pointlight_position);

  auto const& attenuation = pointlight.light.attenuation;
  auto const attenuation_field = [&make_field](char const* fieldname) {
    return make_field("attenuation.") + fieldname;
  };
  auto const constant = attenuation_field("constant");
  auto const linear = attenuation_field("linear");
  auto const quadratic = attenuation_field("quadratic");
  sp.set_uniform_float1(logger, constant,  attenuation.constant);
  sp.set_uniform_float1(logger, linear,    attenuation.linear);
  sp.set_uniform_float1(logger, quadratic, attenuation.quadratic);
}

struct PointlightTransform
{
  Transform const& transform;
  PointLight const& pointlight;
};

void
set_receiveslight_uniforms(RenderState &rstate, glm::mat4 const& model_matrix,
    ShaderProgram &sp, DrawInfo const& dinfo, Material const& material,
    std::vector<PointlightTransform> const& pointlights, bool const receives_ambient_light)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;
  auto &lstate = zs.level_state;

  auto &logger = es.logger;
  auto const& camera = lstate.camera;
  auto const& global_light = lstate.global_light;
  auto const& player = lstate.player;

  set_modelmatrix(logger, model_matrix, sp);
  sp.set_uniform_matrix_3fv(logger, "u_normalmatrix", glm::inverseTranspose(glm::mat3{model_matrix}));

  //set_dirlight(logger, sp, global_light);

  // ambient
  if (receives_ambient_light) {
    sp.set_uniform_color_3fv(logger, "u_globallight.ambient", global_light.ambient);
  }

  // specular
  sp.set_uniform_float1(logger, "u_reflectivity", 1.0f);

  // pointlight
  sp.set_uniform_matrix_4fv(logger, "u_invviewmatrix", glm::inverse(glm::mat3{camera.view_matrix()}));

  FOR(i, pointlights.size()) {
    auto const& transform = pointlights[i].transform;
    auto const& pointlight = pointlights[i].pointlight;
    set_pointlight(logger, sp, i, pointlight, transform.translation);
  }

  sp.set_uniform_vec3(logger, "u_material.ambient",  material.ambient);
  sp.set_uniform_vec3(logger, "u_material.diffuse",  material.diffuse);
  sp.set_uniform_vec3(logger, "u_material.specular", material.specular);
  sp.set_uniform_float1(logger, "u_material.shininess", material.shininess);

  sp.set_uniform_bool(logger, "u_drawnormals", es.draw_normals);
  // TODO: when re-implementing LOS restrictions
  //sp.set_uniform_vec3(logger, "u_player.position",  player.world_position());
  //sp.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
  //sp.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));
}

void
draw_3dshape(RenderState &rstate, glm::mat4 const& model_matrix, ShaderProgram &sp,
    DrawInfo const& dinfo)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;
  auto &lstate = zs.level_state;

  auto &logger = es.logger;
  auto const& camera = lstate.camera;

  // various matrices
  set_mvpmatrix(logger, model_matrix, sp, camera);

  if (sp.is_skybox) {
    disable_depth_tests();
    draw_drawinfo(logger, sp, dinfo);
    enable_depth_tests();
  } else {
    draw_drawinfo(logger, sp, dinfo);
  }
}

void
draw_3dlit_shape(RenderState &rstate, glm::mat4 const& model_matrix, ShaderProgram &sp,
  DrawInfo const& dinfo, Material const& material, EntityRegistry &registry,
  bool const receives_ambient_light)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  auto const pointlight_eids = find_pointlights(registry);
  std::vector<PointlightTransform> pointlights;

  FOR(i, pointlight_eids.size()) {
    auto const& eid = pointlight_eids[i];
    auto &transform = registry.get<Transform>(eid);
    auto &pointlight = registry.get<PointLight>(eid);
    PointlightTransform const plt{transform, pointlight};

    pointlights.emplace_back(plt);
  }
  set_receiveslight_uniforms(rstate, model_matrix, sp, dinfo, material, pointlights,
      receives_ambient_light);
  draw_3dshape(rstate, model_matrix, sp, dinfo);
}

void
draw_3dlightsource(RenderState &rstate, glm::mat4 const& model_matrix, ShaderProgram &sp,
  DrawInfo const& dinfo, EntityID const entity, EntityRegistry &registry)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  auto &logger = es.logger;
  auto &pointlight = registry.get<PointLight>(entity);

  auto const diffuse = pointlight.light.diffuse;
  sp.set_uniform_color_3fv(logger, "u_lightcolor", diffuse);
  draw_3dshape(rstate, model_matrix, sp, dinfo);
}

void
draw(RenderState &rstate, Transform const& transform, ShaderProgram &sp,
    DrawInfo const& dinfo, EntityID const entity, EntityRegistry &registry)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;
  auto &logger = es.logger;

  // Use the sp's PROGRAM and bind the sp's VAO.
  sp.use(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

  if (sp.is_2d) {
    disable_depth_tests();
    draw_drawinfo(logger, sp, dinfo);
    enable_depth_tests();
    return;
  }

  bool const is_lightsource = registry.has<PointLight>(entity);
  auto const model_matrix = transform.model_matrix();
  if (is_lightsource) {
    assert(is_lightsource);
    draw_3dlightsource(rstate, model_matrix, sp, dinfo, entity, registry);
    return;
  }
  // Only true for now, old code had this set.
  bool constexpr receives_ambient_light = true;

  bool const receives_light = registry.has<Material>(entity);
  if (receives_light) {
    assert(registry.has<Material>(entity));
    Material const& material = registry.get<Material>(entity);
    draw_3dlit_shape(rstate, model_matrix, sp, dinfo, material, registry,
        receives_ambient_light);
    return;
  }

  // Can't receive light
  assert(!registry.has<Material>());
  draw_3dshape(rstate, model_matrix, sp, dinfo);
}

} // ns anonymous

namespace boomhs::render
{

void
init(window::Dimensions const& dimensions)
{
  // Initialize opengl
  glViewport(0, 0, dimensions.w, dimensions.h);

  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glDisable(GL_BLEND);

  enable_depth_tests();
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
conditionally_draw_player_vectors(RenderState &rstate, WorldObject const &player)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  auto &logger = es.logger;

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
draw_arrow(RenderState &rstate, glm::vec3 const& start, glm::vec3 const& head, Color const& color)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  auto &logger = es.logger;
  auto &registry = zs.registry;

  auto &sps = zs.gfx_state.sps;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto const handle = OF::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });
  auto &transform = registry.assign<Transform>(entity);

  draw(rstate, transform, sp, handle, entity, registry);
}

void
draw_arrow_abovetile_and_neighbors(RenderState &rstate, TilePosition const& tpos)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;
  auto &lstate = zs.level_state;

  glm::vec3 constexpr offset{0.5f, 2.0f, 0.5f};

  auto const draw_the_arrow = [&](auto const& ntpos, auto const& color) {
    auto const bottom = glm::vec3{ntpos.x + offset.x, offset.y, ntpos.y + offset.y};
    auto const top = bottom + (Y_UNIT_VECTOR * 2.0f);

    draw_arrow(rstate, top, bottom, color);
  };

  draw_the_arrow(tpos, LOC::BLUE);

  auto &leveldata = lstate.level_data;
  auto const& tgrid = leveldata.tilegrid();
  auto const neighbors = find_immediate_neighbors(tgrid, tpos, TileLookupBehavior::ALL_8_DIRECTIONS,
      [](auto const& tpos) { return true; });
  assert(neighbors.size() <= 8);
  FOR(i, neighbors.size()) {
    draw_the_arrow(neighbors[i], LOC::LIME_GREEN);
  }
}

void
draw_global_axis(RenderState &rstate, EntityRegistry &registry)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;
  auto &sps = zs.gfx_state.sps;

  auto &logger = es.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto world_arrows = OF::create_axis_arrows(logger, sp);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);

  draw(rstate, transform, sp, world_arrows.x_dinfo, entity, registry);
  draw(rstate, transform, sp, world_arrows.y_dinfo, entity, registry);
  draw(rstate, transform, sp, world_arrows.z_dinfo, entity, registry);
}

void
draw_local_axis(RenderState &rstate, EntityRegistry &registry, glm::vec3 const &player_pos)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;
  auto &sps = zs.gfx_state.sps;

  auto &logger = es.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto const axis_arrows = OF::create_axis_arrows(logger, sp);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);
  transform.translation = player_pos;

  draw(rstate, transform, sp, axis_arrows.x_dinfo, entity, registry);
  draw(rstate, transform, sp, axis_arrows.y_dinfo, entity, registry);
  draw(rstate, transform, sp, axis_arrows.z_dinfo, entity, registry);
}

void
draw_entities(RenderState &rstate, stlw::float_generator &rng, FrameTime const& ft)
{
  auto const& es = rstate.es;
  auto &zs = rstate.zs;

  assert(zs.gfx_state.gpu_state.entities);
  auto &entity_handles = *zs.gfx_state.gpu_state.entities;

  auto &registry = zs.registry;
  auto &sps = zs.gfx_state.sps;

  auto &ldata = zs.level_state;
  auto &camera = ldata.camera;
  auto &player = ldata.player;

  auto const draw_fn = [&entity_handles, &sps, &rstate, &registry](auto eid, auto &sn,
      auto &transform, auto &&...)
  {
    auto &sp = sps.ref_sp(sn.value);
    auto &handle = entity_handles.lookup(eid);
    draw(rstate, transform, sp, handle, eid, registry);
  };

  auto const player_drawfn = [&camera, &draw_fn](auto &&... args)
  {
    if (CameraMode::FPS == camera.mode()) {
      return;
    }
    draw_fn(FORWARD(args)...);
  };
  auto const enemy_drawfn = [&draw_fn](auto eid, auto &enemy, auto &&...args)
  {
    if (!enemy.is_visible) {
      return;
    }
    draw_fn(eid, FORWARD(args)...);
  };

  auto const draw_torch = [&draw_fn, &rng](auto eid, auto &sn, auto &transform, auto &&... args)
  {
    // randomize the position slightly
    auto static constexpr DISPLACEMENT_MAX = 0.015f;
    auto copy_transform = transform;
    copy_transform.translation.x += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.y += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);
    copy_transform.translation.z += rng.gen_float_range(-DISPLACEMENT_MAX, DISPLACEMENT_MAX);

    draw_fn(eid, sn, transform, FORWARD(args)...);
  };

  //
  // Draw the cubes
  registry.view<ShaderName, Transform, EntityFromFILE>().each(draw_fn);
  registry.view<ShaderName, Transform, PointLight>().each(draw_torch);

  registry.view<Enemy, ShaderName, Transform, MeshRenderable>().each(enemy_drawfn);
  registry.view<ShaderName, Transform, MeshRenderable, Player>().each(player_drawfn);

  // Draw the tiles
  registry.view<ShaderName, Transform, MeshRenderable, TileComponent>().each(draw_fn);

  if (es.draw_skybox) {
    auto const draw_skybox = [&](auto eid, auto &sn, auto &transform, auto &) {
      draw_fn(eid, sn, transform);
    };
    registry.view<ShaderName, Transform, SkyboxRenderable>().each(draw_skybox);
  }
}

void
draw_tilegrid(RenderState &rstate, TiledataState const& tilegrid_state, FrameTime const& ft)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;
  auto &lstate = zs.level_state;

  auto &logger = es.logger;
  assert(zs.gfx_state.gpu_state.tiles);
  auto &tile_handles = *zs.gfx_state.gpu_state.tiles;
  auto &registry = zs.registry;
  auto &sps = zs.gfx_state.sps;

  auto const& leveldata = lstate.level_data;
  auto const& tilegrid = leveldata.tilegrid();
  auto const& tiletable = leveldata.tiletable();

  auto const& draw_tile_helper = [&](auto &sp, auto const& dinfo, Tile const& tile,
      glm::mat4 const& model_mat, bool const receives_ambient_light)
  {
    auto const& tileinfo = tiletable[tile.type];
    auto const& material = tileinfo.material;

    sp.use(logger);
    opengl::global::vao_bind(dinfo.vao());
    ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });
    draw_3dlit_shape(rstate, model_mat, sp, dinfo, material, registry, receives_ambient_light);
  };
  auto const draw_tile = [&](auto const& tile_pos) {
    auto const& tile = tilegrid.data(tile_pos);
    if (!tilegrid_state.reveal && !tile.is_visible) {
      return;
    }
    // This offset causes the tile's to appear in the "middle"
    auto &tile_sp = sps.ref_sp("3d_pos_normal_color");

    auto const tr = tile_pos + VIEWING_OFFSET;
    auto &transform = registry.get<Transform>(tile.eid);
    auto const& rotation   = transform.rotation;
    auto const default_modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, transform.scale);
    auto const& dinfo = tile_handles.lookup(tile.type);

    switch(tile.type) {
      case TileType::FLOOR:
        {
          auto &floor_sp = sps.ref_sp("floor");
          auto const scale = glm::vec3{0.8};
          auto const modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, scale);
          draw_tile_helper(floor_sp, dinfo, tile, modmatrix, true);
        }
        break;
      case TileType::WALL:
        {
          auto const inverse_model = glm::inverse(default_modmatrix);
          auto &sp = sps.ref_sp("hashtag");
          sp.set_uniform_matrix_4fv(logger, "u_inversemodelmatrix", inverse_model);
          draw_tile_helper(sp, dinfo, tile, default_modmatrix, true);
        }
        break;
      case TileType::RIVER:
        // Do nothing, we handle rendering rivers elsewhere.
        break;
      case TileType::BRIDGE:
        {
          // TODo: how to orient bridge (tile) based on RiverInfo (nontile) information?
          //
          // Previously we haven't read data stored outside the EntityRegistry when
          // rendering tiles.
          //
          // thinking ...
          draw_tile_helper(tile_sp, dinfo, tile, default_modmatrix, true);
        }
        break;
      case TileType::STAIR_DOWN:
        {
          auto &sp = sps.ref_sp("stair");
          sp.set_uniform_color(logger, "u_color", LOC::WHITE);

          bool const receives_ambient_light = false;
          draw_tile_helper(sp, dinfo, tile, default_modmatrix, receives_ambient_light);
        }
        break;
      case TileType::STAIR_UP:
        {
          auto &sp = sps.ref_sp("stair");
          sp.set_uniform_color(logger, "u_color", LOC::WHITE);

          bool const receives_ambient_light = false;
          draw_tile_helper(sp, dinfo, tile, default_modmatrix, receives_ambient_light);
        }
        break;
      default:
        std::exit(1);
    }
  };
  tilegrid.visit_each(draw_tile);
}

void
draw_targetreticle(RenderState &rstate, window::FrameTime const& ft)
{
  auto &zs = rstate.zs;
  auto &ldata = zs.level_state;

  auto const& nearby_targets = ldata.nearby_targets;
  if (nearby_targets.empty()) {
    return;
  }
  auto const nearest_enemy = nearby_targets.closest();

  auto &registry = zs.registry;
  auto eid = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(eid); });

  auto &transform = registry.assign<Transform>(eid);
  assert(registry.has<Transform>(eid));

  assert(registry.has<Transform>(nearest_enemy));
  auto &enemy_transform = registry.get<Transform>(nearest_enemy);
  transform.translation = enemy_transform.translation;

  auto &camera = zs.level_state.camera;
  assert(registry.has<Transform>(eid));

  auto texture_o = zs.gfx_state.texture_table.find("TargetReticle");
  assert(texture_o);

  auto &sps = zs.gfx_state.sps;
  auto &sp = sps.ref_sp("2dtexture");

  auto &logger = rstate.es.logger;
  DrawInfo di = gpu::copy_rectangle_uvs(logger, sp, texture_o);

  auto model_matrix = transform.model_matrix();
  auto viewmodel_matrix = camera.view_matrix() * model_matrix;

  // Reset the rotation values in order to achieve a billboard effect.
  //
  // http://www.geeks3d.com/20140807/billboarding-vertex-shader-glsl/
  float *data = glm::value_ptr(viewmodel_matrix);
  data[1] = 0.0f;
  data[2] = 0.0f;
  data[4] = 0.0f;
  data[6] = 0.0f;
  data[8] = 0.0f;
  data[9] = 0.0f;

  // Set the scale values
  auto constexpr SCALE = 0.60f;
  data[0] = SCALE;
  data[5] = SCALE;
  data[10] = SCALE;

  auto constexpr ROTATE_SPEED = 50.0f;
  float const angle = ROTATE_SPEED * ft.since_start_seconds();
  auto const rot = glm::angleAxis(glm::radians(angle), Z_UNIT_VECTOR);
  auto const rmatrix = glm::toMat4(rot);
  viewmodel_matrix = viewmodel_matrix * rmatrix;

  auto const mvp_matrix = camera.projection_matrix() * viewmodel_matrix;
  set_modelmatrix(logger, mvp_matrix, sp);
  draw(rstate, transform, sp, di, eid, registry);
}

void
draw_rivers(RenderState &rstate, window::FrameTime const& ft)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  assert(zs.gfx_state.gpu_state.tiles);
  auto &tile_handles = *zs.gfx_state.gpu_state.tiles;
  auto &registry = zs.registry;
  auto &sps = zs.gfx_state.sps;

  auto &sp = sps.ref_sp("river");
  auto const& dinfo = tile_handles.lookup(TileType::RIVER);

  opengl::global::vao_bind(dinfo.vao());
  sp.set_uniform_color(es.logger, "u_color", LOC::WHITE);

  auto const& level_data = zs.level_state.level_data;
  auto const& tile_info = level_data.tiletable()[TileType::RIVER];
  auto const& material = tile_info.material;

  auto const draw_river = [&](auto const& rinfo) {
    auto const& left = rinfo.left;
    auto const& right = rinfo.right;

    auto const draw_wiggle = [&](auto const& wiggle) {
      sp.set_uniform_vec2(es.logger, "u_direction", wiggle.direction);
      sp.set_uniform_vec2(es.logger, "u_offset", wiggle.offset);

      auto const& wp = wiggle.position;
      auto const tr = glm::vec3{wp.x, WIGGLE_UNDERATH_OFFSET, wp.y} + VIEWING_OFFSET;
      glm::quat const rot = glm::angleAxis(glm::degrees(rinfo.wiggle_rotation), Y_UNIT_VECTOR);
      auto const scale = glm::vec3{0.5};

      bool const receives_ambient = true;
      auto const modelmatrix = stlw::math::calculate_modelmatrix(tr, rot, scale);
      auto const inverse_model = glm::inverse(modelmatrix);
      sp.set_uniform_matrix_4fv(es.logger, "u_inversemodelmatrix", inverse_model);
      draw_3dlit_shape(rstate, modelmatrix, sp, dinfo, material, registry, receives_ambient);
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
draw_stars(RenderState &rstate, window::FrameTime const& ft)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  assert(zs.gfx_state.gpu_state.tiles);
  auto &tile_handles = *zs.gfx_state.gpu_state.tiles;
  auto &registry = zs.registry;
  auto &sps = zs.gfx_state.sps;

  auto const draw_starletter = [&](int const x, int const y, char const* shader, TileType const type)
  {
    auto &sp = sps.ref_sp(shader);
    sp.set_uniform_color_3fv(es.logger, "u_lightcolor", LOC::YELLOW);

    auto const& dinfo = tile_handles.lookup(type);
    opengl::global::vao_bind(dinfo.vao());

    auto constexpr Z = 5.0f;
    auto const tr = glm::vec3{x, y, Z};
    glm::quat const rot = glm::angleAxis(glm::radians(90.0f), Z_UNIT_VECTOR);

    static constexpr double MIN = 0.3;
    static constexpr double MAX = 1.0;
    static constexpr double SPEED = 0.25;
    auto const a = std::sin(ft.since_start_seconds() * M_PI  * SPEED);
    float const scale = glm::lerp(MIN, MAX, std::abs(a));

    auto const& level_data = zs.level_state.level_data;
    auto const& tile_info = level_data.tiletable()[type];
    auto const& material = tile_info.material;

    auto const scalevec = glm::vec3{scale};
    auto const modelmatrix = stlw::math::calculate_modelmatrix(tr, rot, scalevec);
    draw_3dshape(rstate, modelmatrix, sp, dinfo);
  };

  auto constexpr X = -15.0;
  auto constexpr Y = 5.0;
  draw_starletter(X, Y,   "light", TileType::STAR);
  draw_starletter(X, Y+1, "light", TileType::BAR);
}

void
draw_terrain(RenderState &rstate)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  auto &registry = zs.registry;
  auto &sps = zs.gfx_state.sps;

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &terrain_sp = sps.ref_sp("3d_pos_normal_color");
  auto terrain = gpu::copy_normalcolorcube_gpu(es.logger, terrain_sp, LOC::WHITE);

  auto &transform = registry.assign<Transform>(entity);
  auto &scale = transform.scale;
  scale.x = 50.0f;
  scale.y = 0.2f;
  scale.z = 50.0f;
  auto &translation = transform.translation;
  // translation.x = 3.0f;
  translation.y = -2.0f;
  // translation.z = 2.0f;
  draw(rstate, transform, terrain_sp, terrain, entity, registry);
}

void
draw_tilegrid(RenderState &rstate, TiledataState const& tds)
{
  auto &es = rstate.es;
  auto &zs = rstate.zs;

  auto &logger = es.logger;
  auto &sps = zs.gfx_state.sps;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto &lstate = zs.level_state;
  auto const& leveldata = lstate.level_data;
  auto const& tilegrid = leveldata.tilegrid();

  Transform transform;
  bool const show_y = tds.show_yaxis_lines;
  auto const dinfo = OF::create_tilegrid(es.logger, sp, tilegrid, show_y);

  set_mvpmatrix(logger, transform.model_matrix(), sp, lstate.camera);

  sp.use(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });
  draw_drawinfo(logger, sp, dinfo);
}

} // ns boomhs::render
