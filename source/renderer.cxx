#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tiledata.hpp>
#include <boomhs/tiledata_algorithms.hpp>
#include <boomhs/types.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/global.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>

#include <window/timer.hpp>

#include <stlw/math.hpp>
#include <stlw/log.hpp>
#include <iostream>

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

  if (sp.instance_count) {
    auto const ic = *sp.instance_count;
    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, ic);
  } else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
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
set_pointlight(stlw::Logger &logger, ShaderProgram &sp, std::size_t const index,
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

void
set_receiveslight_uniforms(boomhs::RenderArgs const &args, glm::mat4 const& model_matrix,
    ShaderProgram &sp, DrawInfo const& dinfo, Material const& material, std::uint32_t const entity,
    entt::DefaultRegistry &registry, bool const receives_ambient_light)
{
  auto const& camera = args.camera;
  auto const& global_light = args.global_light;
  auto const& player = args.player;
  auto &logger = args.logger;

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
  auto const pointlights = find_pointlights(registry);
  FOR(i, pointlights.size()) {
    auto const& entity = pointlights[i];
    auto &transform = registry.get<Transform>(entity);
    auto &pointlight = registry.get<PointLight>(entity);
    set_pointlight(logger, sp, i, pointlight, transform.translation);
  }

  sp.set_uniform_vec3(logger, "u_material.ambient",  material.ambient);
  sp.set_uniform_vec3(logger, "u_material.diffuse",  material.diffuse);
  sp.set_uniform_vec3(logger, "u_material.specular", material.specular);
  sp.set_uniform_float1(logger, "u_material.shininess", material.shininess);

  sp.set_uniform_bool(logger, "u_drawnormals", args.draw_normals);
  // TODO: when re-implementing LOS restrictions
  //sp.set_uniform_vec3(logger, "u_player.position",  player.world_position());
  //sp.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
  //sp.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));
}

void
draw_3dshape(RenderArgs const &args, glm::mat4 const& model_matrix, ShaderProgram &sp,
    DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  auto const& camera = args.camera;

  auto const draw_3d_shape_fn = [&]()
  {
    // various matrices
    set_mvpmatrix(logger, model_matrix, sp, camera);

    if (sp.is_skybox) {
      disable_depth_tests();
      draw_drawinfo(logger, sp, dinfo);
      enable_depth_tests();
    } else {
      draw_drawinfo(logger, sp, dinfo);
    }
  };
  if (dinfo.texture_info()) {
    auto const ti = *dinfo.texture_info();
    opengl::global::texture_bind(ti);
    ON_SCOPE_EXIT([&ti]() { opengl::global::texture_unbind(ti); });
    draw_3d_shape_fn();
  } else {
    draw_3d_shape_fn();
  }
}

void
draw_3dlit_shape(RenderArgs const &args, glm::mat4 const& model_matrix, ShaderProgram &sp,
  DrawInfo const& dinfo, std::uint32_t const entity, Material const& material,
  entt::DefaultRegistry &registry, bool const receives_ambient_light)
{
  set_receiveslight_uniforms(args, model_matrix, sp, dinfo, material, entity, registry,
      receives_ambient_light);
  draw_3dshape(args, model_matrix, sp, dinfo);
}

void
draw_3dlightsource(RenderArgs const &args, glm::mat4 const& model_matrix, ShaderProgram &sp,
  DrawInfo const& dinfo, std::uint32_t const entity, entt::DefaultRegistry &registry,
  std::vector<std::uint32_t> const& pointlights)
{
  auto &logger = args.logger;
  PointLight *ptr = nullptr;
  FOR(i, pointlights.size()) {
    auto const e = pointlights[i];
    if (entity == e) {
      ptr = &registry.get<PointLight>(e);
      break;
    }
  }
  assert(nullptr != ptr);

  auto const diffuse = ptr->light.diffuse;
  sp.set_uniform_color_3fv(logger, "u_lightcolor", diffuse);
  draw_3dshape(args, model_matrix, sp, dinfo);
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
conditionally_draw_player_vectors(RenderArgs const& rargs, WorldObject const &player,
    EngineState &es, ZoneState &zone_state)
{
  auto &logger = es.logger;

  glm::vec3 const pos = player.world_position();
  if (es.show_player_localspace_vectors) {
    // local-space
    //
    // forward
    auto const fwd = player.eye_forward();
    draw_arrow(rargs, zone_state, pos, pos + fwd, LOC::GREEN);

    // right
    auto const right = player.eye_right();
    draw_arrow(rargs, zone_state, pos, pos + right, LOC::RED);
  }
  if (es.show_player_worldspace_vectors) {
    // world-space
    //
    // forward
    auto const fwd = player.world_forward();
    draw_arrow(rargs, zone_state, pos, pos + (2.0f * fwd), LOC::LIGHT_BLUE);

    // backward
    glm::vec3 const right = player.world_right();
    draw_arrow(rargs, zone_state, pos, pos + right, LOC::PINK);
  }
}

void
draw(RenderArgs const& args, Transform const& transform, ShaderProgram &sp,
    DrawInfo const& dinfo, std::uint32_t const entity, entt::DefaultRegistry &registry)
{
  auto &logger = args.logger;

  // Use the sp's PROGRAM and bind the sp's VAO.
  sp.use(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

  if (sp.is_2d) {
    std::abort(); // We're not using this currently, assert that until no longer true.
    disable_depth_tests();
    set_modelmatrix(logger, transform.model_matrix(), sp);
    enable_depth_tests();
    return;
  }

  bool const is_lightsource = registry.has<PointLight>(entity);
  auto const model_matrix = transform.model_matrix();
  if (is_lightsource) {
    assert(is_lightsource);
    auto const pointlights = find_pointlights(registry);
    draw_3dlightsource(args, model_matrix, sp, dinfo, entity, registry, pointlights);
    return;
  }
  // Only true for now, old code had this set.
  bool constexpr receives_ambient_light = true;

  bool const receives_light = registry.has<Material>(entity);
  if (receives_light) {
    assert(registry.has<Material>(entity));
    Material const& material = registry.get<Material>(entity);
    draw_3dlit_shape(args, model_matrix, sp, dinfo, entity, material, registry,
        receives_ambient_light);
    return;
  }

  // Can't receive light
  assert(!registry.has<Material>());
  draw_3dshape(args, model_matrix, sp, dinfo);
}

void
draw_arrow(RenderArgs const& rargs, ZoneState &zone_state, glm::vec3 const& start,
    glm::vec3 const& head, Color const& color)
{
  auto &logger = rargs.logger;
  auto &registry = zone_state.registry;

  auto &sps = zone_state.sps;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto const handle = OF::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });
  auto &transform = registry.assign<Transform>(entity);

  draw(rargs, transform, sp, handle, entity, registry);
}

void
draw_arrow_abovetile_and_neighbors(RenderArgs const& rargs, TilePosition const& tpos,
    ZoneState &zone_state)
{
  glm::vec3 constexpr offset{0.5f, 2.0f, 0.5f};

  auto const draw_the_arrow = [&](auto const& ntpos, auto const& color) {
    auto const bottom = glm::vec3{ntpos.x + offset.x, offset.y, ntpos.y + offset.y};
    auto const top = bottom + (Y_UNIT_VECTOR * 2.0f);

    draw_arrow(rargs, zone_state, top, bottom, color);
  };

  draw_the_arrow(tpos, LOC::BLUE);

  auto &leveldata = zone_state.level_data;
  auto const& tdata = leveldata.tiledata();
  auto const neighbors = find_immediate_neighbors(tdata, tpos, TileLookupBehavior::ALL_8_DIRECTIONS,
      [](auto const& tpos) { return true; });
  //assert(neighbors.size() <= 8);
  FOR(i, neighbors.size()) {
    draw_the_arrow(neighbors[i], LOC::LIME_GREEN);
  }
}

void
draw_global_axis(RenderArgs const& rargs, entt::DefaultRegistry &registry, ShaderPrograms &sps)
{
  auto &logger = rargs.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto world_arrows = OF::create_axis_arrows(logger, sp);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);

  draw(rargs, transform, sp, world_arrows.x_dinfo, entity, registry);
  draw(rargs, transform, sp, world_arrows.y_dinfo, entity, registry);
  draw(rargs, transform, sp, world_arrows.z_dinfo, entity, registry);
}

void
draw_local_axis(RenderArgs const& rargs, entt::DefaultRegistry &registry, ShaderPrograms &sps,
    glm::vec3 const &player_pos)
{
  auto &logger = rargs.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto const axis_arrows = OF::create_axis_arrows(logger, sp);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);
  transform.translation = player_pos;

  draw(rargs, transform, sp, axis_arrows.x_dinfo, entity, registry);
  draw(rargs, transform, sp, axis_arrows.y_dinfo, entity, registry);
  draw(rargs, transform, sp, axis_arrows.z_dinfo, entity, registry);
}

void
draw_entities(RenderArgs const& rargs, EngineState const& es, ZoneState &zone_state)
{
  auto &handlem = zone_state.handles;
  auto &registry = zone_state.registry;
  auto &sps = zone_state.sps;

  auto const draw_fn = [&handlem, &sps, &rargs, &registry](auto entity, auto &sn, auto &transform) {
    auto &shader_ref = sps.ref_sp(sn.value);
    auto &handle = handlem.lookup(entity);
    draw(rargs, transform, shader_ref, handle, entity, registry);
  };

  auto const draw_adapter = [&](auto entity, auto &sn, auto &transform, auto &) {
    draw_fn(entity, sn, transform);
  };

  //
  // Actual drawing begins here
  registry.view<ShaderName, Transform, CubeRenderable>().each(draw_adapter);
  registry.view<ShaderName, Transform, MeshRenderable>().each(draw_adapter);

  if (es.draw_skybox) {
    auto const draw_skybox = [&](auto entity, auto &sn, auto &transform, auto &) {
      draw_fn(entity, sn, transform);
    };
    registry.view<ShaderName, Transform, SkyboxRenderable>().each(draw_skybox);
  }
}

void
draw_tiledata(RenderArgs const& args, TiledataState const& tiledata_state, ZoneState &zone_state,
    FrameTime const& ft)
{
  auto &logger = args.logger;
  auto &handlem = zone_state.handles;
  auto &registry = zone_state.registry;
  auto &sps = zone_state.sps;

  auto const& leveldata = zone_state.level_data;
  auto const& tiledata = leveldata.tiledata();

  auto const& draw_tile_helper = [&](auto &sp, auto const& dinfo, std::uint32_t const entity,
      glm::mat4 const& model_mat, bool const receives_ambient_light)
  {
    sp.use(logger);
    opengl::global::vao_bind(dinfo.vao());
    ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

    auto const& level_data = zone_state.level_data;
    auto const& tileinfo = level_data.tileinfos()[TileType::RIVER];
    auto const& material = tileinfo.material;
    draw_3dlit_shape(args, model_mat, sp, dinfo, entity, material, registry, receives_ambient_light);
  };
  auto const draw_tile = [&](auto const& tile_pos) {
    auto const& tile = tiledata.data(tile_pos);
    if (!tiledata_state.reveal && !tile.is_visible) {
      return;
    }
    // This offset causes the tile's to appear in the "middle"
    auto &tile_sp = sps.ref_sp("3d_pos_normal_color");

    auto const tr = tile_pos + VIEWING_OFFSET;
    auto &transform = registry.get<Transform>(tile.eid);
    auto const& rotation   = transform.rotation;
    auto const default_modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, transform.scale);
    switch(tile.type) {
      case TileType::FLOOR:
        {
          auto const& plus_dinfo = handlem.lookup(handlem.plus_eid);
          draw_tile_helper(tile_sp, plus_dinfo, tile.eid, default_modmatrix, true);
        }
        break;
      case TileType::WALL:
        {
          auto &hashtag_sp = sps.ref_sp("hashtag");
          auto const& hashtag_dinfo = handlem.lookup(handlem.hashtag_eid);
          draw_tile_helper(hashtag_sp, hashtag_dinfo, tile.eid, default_modmatrix, true);
        }
        break;
      case TileType::RIVER:
        // Do nothing, we handle rendering rivers elsewhere.
        break;
      case TileType::BRIDGE:
        {
          // TODo: how to orient bridge (tile) based on RiverInfo (nontile) information?
          //
          // Previously we haven't read data stored outside the entt::DefaultRegistry when
          // rendering tiles.
          //
          // thinking ...
          auto const& bridge_dinfo = handlem.lookup(handlem.bridge_eid);
          auto const modmatrix = stlw::math::calculate_modelmatrix(tr, rotation, transform.scale);
          draw_tile_helper(tile_sp, bridge_dinfo, tile.eid, default_modmatrix, true);
        }
        break;
      case TileType::STAIR_DOWN:
        {
          auto &sp = sps.ref_sp("stair");
          sp.set_uniform_color(logger, "u_color", LOC::WHITE);

          auto const& stair_downdinfo = handlem.lookup(handlem.stairdown_eid);
          bool const receives_ambient_light = false;
          draw_tile_helper(sp, stair_downdinfo, tile.eid, default_modmatrix, receives_ambient_light);
        }
        break;
      case TileType::STAIR_UP:
        {
          auto &sp = sps.ref_sp("stair");
          sp.set_uniform_color(logger, "u_color", LOC::WHITE);

          auto const& stair_updinfo = handlem.lookup(handlem.stairup_eid);
          bool const receives_ambient_light = false;
          draw_tile_helper(sp, stair_updinfo, tile.eid, default_modmatrix, receives_ambient_light);
        }
        break;
      default:
        std::exit(1);
    }
  };
  tiledata.visit_each(draw_tile);
}

void
draw_rivers(RenderArgs const& rargs, ZoneState &zone_state, window::FrameTime const& ft)
{

  auto &handlem = zone_state.handles;
  auto &registry = zone_state.registry;
  auto &sps = zone_state.sps;

  auto &sp = sps.ref_sp("river");
  auto const eid = handlem.river_eid;
  auto const& dinfo = handlem.lookup(eid);

  sp.set_uniform_color(rargs.logger, "u_color", LOC::WHITE);
  opengl::global::vao_bind(dinfo.vao());

  auto const& level_data = zone_state.level_data;
  auto const& tileinfo = level_data.tileinfos()[TileType::RIVER];
  auto const& material = tileinfo.material;

  auto const draw_river = [&](auto const& rinfo) {
    auto const& left = rinfo.left;
    auto const& right = rinfo.right;

    auto const draw_wiggle = [&](auto const& wiggle) {
      sp.set_uniform_vec2(rargs.logger, "u_direction", wiggle.direction);
      sp.set_uniform_vec2(rargs.logger, "u_offset", wiggle.offset);

      auto const& wp = wiggle.position;
      auto const tr = glm::vec3{wp.x, WIGGLE_UNDERATH_OFFSET, wp.y} + VIEWING_OFFSET;
      glm::quat const rot = glm::angleAxis(glm::degrees(rinfo.wiggle_rotation), Y_UNIT_VECTOR);
      auto const scale = glm::vec3{0.5};

      bool const receives_ambient = true;
      auto const modelmatrix = stlw::math::calculate_modelmatrix(tr, rot, scale);
      draw_3dlit_shape(rargs, modelmatrix, sp, dinfo, eid, material, registry, receives_ambient);
    };
    for (auto const& w : rinfo.wiggles) {
      draw_wiggle(w);
    }
  };
  auto const& rinfos = zone_state.level_data.rivers();
  for (auto const& rinfo : rinfos) {
    draw_river(rinfo);
  }
}

void
draw_terrain(RenderArgs const& rargs, ZoneState &zone_state)
{
  auto &registry = zone_state.registry;
  auto &sps = zone_state.sps;

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &terrain_sp = sps.ref_sp("3d_pos_normal_color");
  auto terrain = OF::copy_normalcolorcube_gpu(rargs.logger, terrain_sp, LOC::WHITE);

  auto &transform = registry.assign<Transform>(entity);
  auto &scale = transform.scale;
  scale.x = 50.0f;
  scale.y = 0.2f;
  scale.z = 50.0f;
  auto &translation = transform.translation;
  // translation.x = 3.0f;
  translation.y = -2.0f;
  // translation.z = 2.0f;
  draw(rargs, transform, terrain_sp, terrain, entity, registry);
}

void
draw_tilegrid(RenderArgs const& args, TiledataState const& tds, ZoneState &zone_state)
{
  auto &logger = args.logger;
  auto &sps = zone_state.sps;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto const& leveldata = zone_state.level_data;
  auto const& tiledata = leveldata.tiledata();

  Transform transform;
  bool const show_y = tds.show_yaxis_lines;
  auto const dinfo = OF::create_tilegrid(args.logger, sp, tiledata, show_y);

  set_mvpmatrix(logger, transform.model_matrix(), sp, args.camera);

  sp.use(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });
  draw_drawinfo(logger, sp, dinfo);
}

} // ns boomhs::render
