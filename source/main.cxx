#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/components.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_generator.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tiledata.hpp>
#include <boomhs/tiledata_algorithms.hpp>
#include <boomhs/ui.hpp>
#include <boomhs/zone.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>

#include <boost/algorithm/string.hpp>
#include <stlw/optional.hpp>

#include <cassert>
#include <cstdlib>
#include <string>

namespace OF = opengl::factories;
namespace LOC = opengl::LIST_OF_COLORS;
using namespace boomhs;
using namespace opengl;
using namespace window;
using stlw::Logger;

namespace boomhs
{

stlw::result<HandleManager, std::string>
copy_assets_gpu(stlw::Logger &logger, ShaderPrograms &sps, entt::DefaultRegistry &registry,
                ObjCache const &obj_cache)
{
  GpuHandleList handle_list;
  /*
  registry.view<ShaderName, Color, CubeRenderable>().each(
      [&](auto entity, auto &sn, auto &color, auto &) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_colorcube_gpu(logger, shader_ref, color);
        handle_list.add(entity, MOVE(handle));
      });
      */
  registry.view<ShaderName, PointLight, CubeRenderable>().each(
      [&](auto entity, auto &sn, auto &pointlight, auto &) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_vertexonlycube_gpu(logger, shader_ref);
        handle_list.add(entity, MOVE(handle));
      });

  registry.view<ShaderName, Color, MeshRenderable>().each(
      [&](auto entity, auto &sn, auto &color, auto &mesh) {
        auto const &obj = obj_cache.get_obj(mesh.name);
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, stlw::none);
        handle_list.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, CubeRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &, auto &texture) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_texturecube_gpu(logger, shader_ref, texture.texture_info);
        handle_list.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, SkyboxRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &, auto &texture) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_texturecube_gpu(logger, shader_ref, texture.texture_info);
        handle_list.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, MeshRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &mesh, auto &texture) {
        auto const &obj = obj_cache.get_obj(mesh.name);
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, texture.texture_info);
        handle_list.add(entity, MOVE(handle));
      });

  registry.view<ShaderName, MeshRenderable>().each([&](auto entity, auto &sn, auto &mesh) {
    auto const &obj = obj_cache.get_obj(mesh.name);
    auto &shader_ref = sps.ref_sp(sn.value);
    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, stlw::none);
    handle_list.add(entity, MOVE(handle));
  });

  auto const make_special = [&handle_list, &logger, &obj_cache, &sps, &registry](char const *mesh_name, char const*vshader_name) {
    auto const &obj = obj_cache.get_obj(mesh_name);
    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, sps.ref_sp(vshader_name), obj, stlw::none);
    auto const entity = registry.create();
    registry.assign<Material>(entity);
    auto meshc = registry.assign<MeshRenderable>(entity);
    meshc.name = mesh_name;

    auto &color = registry.assign<Color>(entity);
    color.set_r(1.0);
    color.set_g(1.0);
    color.set_b(1.0);
    color.set_a(1.0);

    handle_list.add(entity, MOVE(handle));
    return entity;
  };
  auto const make_stair = [&](char const* vshader_name, char const* mesh_name) {
    auto const &obj = obj_cache.get_obj(mesh_name);
    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, sps.ref_sp(vshader_name), obj, stlw::none);
    auto const eid = registry.create();
    registry.assign<Material>(eid);
    auto meshc = registry.assign<MeshRenderable>(eid);
    meshc.name = mesh_name;

    handle_list.add(eid, MOVE(handle));
    return eid;
  };

  auto const bridge_eid = make_special("B", "3d_pos_normal_color");
  auto const equal_eid = make_special("equal", "3d_pos_normal_color");
  auto const plus_eid = make_special("plus", "3d_pos_normal_color");

  auto const hashtag_eid = make_special("hashtag", "hashtag");
  auto const river_eid = make_special("tilde", "river");
  auto const stair_down_eid = make_stair("stair", "stair_down");
  auto const stair_up_eid = make_stair("stair", "stair_up");

  return HandleManager{MOVE(handle_list), MOVE(bridge_eid), MOVE(equal_eid), plus_eid, hashtag_eid,
    river_eid, stair_down_eid, stair_up_eid};
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
    render::draw(rargs, transform, shader_ref, handle, entity, registry);
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
  render::draw(rargs, transform, terrain_sp, terrain, entity, registry);
}

void
draw_tiledata(RenderArgs const& rargs, ZoneState &zone_state, TiledataState const& tds,
    FrameTime const& ft)
{
  // TODO: How do we "GET" the hashtag and plus shaders under the new entity management system?
  //
  // PROBLEM:
  //    We currently store one draw handle to every entity. For the TileData, we need to store two
  //    (an array) of DrawInfo instances to the one entity.
  //
  // THOUGHT:
  //    It probably makes sense to render the tiledata differently now. We should assume more than
  //    just two tile type's will be necessary, and will have to devise a strategy for quickly
  //    rendering the tiledata using different tile's. Maybe store the different tile type's
  //    together somehow for rendering?
  auto &handlem = zone_state.handles;
  auto &sps = zone_state.sps;

  using namespace render;
  auto &tile_sp = sps.ref_sp("3d_pos_normal_color");

  DrawTileArgs bridge{tile_sp, handlem.lookup(handlem.bridge_eid), handlem.bridge_eid};
  DrawTileArgs equal{tile_sp, handlem.lookup(handlem.equal_eid), handlem.equal_eid};
  DrawTileArgs plus{tile_sp, handlem.lookup(handlem.plus_eid), handlem.plus_eid};

  auto &hashtag_sp = sps.ref_sp("hashtag");
  DrawTileArgs hashtag{hashtag_sp, handlem.lookup(handlem.hashtag_eid), handlem.hashtag_eid};
  DrawTileArgs river{sps.ref_sp("river"), handlem.lookup(handlem.river_eid), handlem.river_eid};

  auto &stair_sp = sps.ref_sp("stair");
  DrawTileArgs stairs_down{stair_sp, handlem.lookup(handlem.stair_down_eid), handlem.stair_down_eid};
  DrawTileArgs stairs_up{stair_sp, handlem.lookup(handlem.stair_up_eid), handlem.stair_up_eid};
  DrawTileDataArgs dta{MOVE(bridge), MOVE(equal), MOVE(plus), MOVE(hashtag), MOVE(river),
    MOVE(stairs_down), MOVE(stairs_up)};

  auto &registry = zone_state.registry;
  auto const& leveldata = zone_state.level_data;
  render::draw_tiledata(rargs, dta, leveldata.tiledata(),
                       tds, registry, ft);
}

void
draw_rivers(RenderArgs const& rargs, ZoneState &zone_state, FrameTime const& ft)
{
  auto &registry = zone_state.registry;
  auto &handlem = zone_state.handles;
  auto &sps = zone_state.sps;

  auto &sp = sps.ref_sp("river");
  auto &river_handle = handlem.lookup(handlem.river_eid);

  auto const& rinfos = zone_state.level_data.rivers();
  for (auto const& rinfo : rinfos) {
    render::draw_rivers(rargs, sp, river_handle, registry, ft, handlem.river_eid, rinfo);
  }
}

void
draw_tilegrid(RenderArgs const& rargs, ZoneState &zone_state, TiledataState const& tds)
{
  auto &sps = zone_state.sps;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto const& leveldata = zone_state.level_data;
  auto const& tiledata = leveldata.tiledata();

  Transform transform;
  bool const show_y = tds.show_yaxis_lines;
  auto const tilegrid = OF::create_tilegrid(rargs.logger, sp, tiledata, show_y);
  render::draw_tilegrid(rargs, transform, sp, tilegrid);
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

  render::draw(rargs, transform, sp, world_arrows.x_dinfo, entity, registry);
  render::draw(rargs, transform, sp, world_arrows.y_dinfo, entity, registry);
  render::draw(rargs, transform, sp, world_arrows.z_dinfo, entity, registry);
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

  render::draw(rargs, transform, sp, axis_arrows.x_dinfo, entity, registry);
  render::draw(rargs, transform, sp, axis_arrows.y_dinfo, entity, registry);
  render::draw(rargs, transform, sp, axis_arrows.z_dinfo, entity, registry);
}

void
draw_arrow(RenderArgs const& rargs, entt::DefaultRegistry &registry, ShaderPrograms &sps,
    glm::vec3 const& start, glm::vec3 const& head, Color const& color)
{
  auto &logger = rargs.logger;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto const handle = OF::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });
  auto &transform = registry.assign<Transform>(entity);

  render::draw(rargs, transform, sp, handle, entity, registry);
}

void
draw_arrow_abovetile_and_neighbors(RenderArgs const& rargs, entt::DefaultRegistry &registry,
    ShaderPrograms &sps, TileData const& tdata, TilePosition const& tpos)
{
  glm::vec3 constexpr offset{0.5f, 2.0f, 0.5f};

  auto const draw_the_arrow = [&](auto const& ntpos, auto const& color) {
    auto const bottom = glm::vec3{ntpos.x + offset.x, offset.y, ntpos.y + offset.y};
    auto const top = bottom + (Y_UNIT_VECTOR * 2.0f);

    draw_arrow(rargs, registry, sps, top, bottom, color);
  };

  draw_the_arrow(tpos, LOC::BLUE);
  auto const neighbors = find_immediate_neighbors(tdata, tpos, TileLookupBehavior::ALL_8_DIRECTIONS,
      [](auto const& tpos) { return true; });
  //assert(neighbors.size() <= 8);
  FOR(i, neighbors.size()) {
    draw_the_arrow(neighbors[i], LOC::LIME_GREEN);
  }
}

void
conditionally_draw_player_vectors(RenderArgs const& rargs, EngineState &es,
    entt::DefaultRegistry &registry, ShaderPrograms &sps, WorldObject const &player)
{
  auto &logger = es.logger;

  glm::vec3 const pos = player.world_position();
  if (es.show_player_localspace_vectors) {
    // local-space
    //
    // forward
    auto const fwd = player.eye_forward();
    draw_arrow(rargs, registry, sps, pos, pos + fwd, LOC::GREEN);

    // right
    auto const right = player.eye_right();
    draw_arrow(rargs, registry, sps, pos, pos + right, LOC::RED);
  }
  if (es.show_player_worldspace_vectors) {
    // world-space
    //
    // forward
    auto const fwd = player.world_forward();
    draw_arrow(rargs, registry, sps, pos, pos + (2.0f * fwd), LOC::LIGHT_BLUE);

    // backward
    glm::vec3 const right = player.world_right();
    draw_arrow(rargs, registry, sps, pos, pos + right, LOC::PINK);
  }
}

void
move_betweentiledatas_ifonstairs(TiledataState &tds, ZoneManager &zm)
{
  auto &zone_state = zm.active();
  auto const& leveldata = zone_state.level_data;

  auto &player = zone_state.player;
  player.transform().translation.y = 0.5f;
  auto const wp = player.world_position();
  {
    auto const [w, h] = leveldata.dimensions();
    assert(wp.x < w);
    assert(wp.y < h);
  }
  auto const& tiledata = leveldata.tiledata();
  auto const& tile = tiledata.data(wp.x, wp.z);
  if (!tile.is_stair()) {
    return;
  }

  auto const move_player_through_stairs = [&tile, &tds, &zm](StairInfo const& stair) {
    {
      int const current = zm.active_zone();
      int const newlevel = current + (tile.is_stair_up() ? 1 : -1);
      //std::cerr << "moving through stair '" << stair.direction << "'\n";
      assert(newlevel < zm.num_zones());
      zm.make_zone_active(newlevel, tds);
    }

    // now that the zone has changed, all references through zm are pointing to old level.
    // use active()
    auto &zone_state = zm.active();

    auto &camera = zone_state.camera;
    auto &player = zone_state.player;
    auto &registry = zone_state.registry;

    auto const spos = stair.exit_position;
    player.move_to(spos.x, player.world_position().y, spos.y);
    player.rotate_to_match_camera_rotation(camera);

    tds.recompute = true;
  };

  // BEGIN
  player.move_to(wp.x, wp.y, wp.z);
  auto const tp = TilePosition::from_floats_truncated(wp.x, wp.z);

  // lookup stairs in the registry
  auto &registry = zone_state.registry;
  auto const stair_eids = find_stairs(registry);
  assert(!stair_eids.empty());

  for (auto const& eid : stair_eids) {
    auto const& stair = registry.get<StairInfo>(eid);
    if (stair.tile_position == tp) {
      move_player_through_stairs(stair);

      // TODO: not just jump first stair we find
      break;
    }
  }
}

void
update_riverwiggles(LevelData &level_data, FrameTime const& ft)
{
  auto const update_river = [&ft](auto &rinfo)
  {
    auto const& left = rinfo.left;
    auto const& right = rinfo.right;
    auto const& top = rinfo.top;
    auto const& bottom = rinfo.bottom;

    for (auto &wiggle : rinfo.wiggles) {
      std::cerr << "wiggle dir: '" << glm::to_string(wiggle.direction) << "'\n";

      auto &pos = wiggle.position;
      pos += wiggle.direction * wiggle.speed * ft.delta;

      if (pos.x > right) {
        pos.x = left;
      }
      if (pos.x < left) {
        pos.x = right;
      }
      if (pos.y < bottom) {
        pos.y = top;
      }
      if (pos.y > top) {
        pos.y = bottom;
      }
    }
  };
  for (auto &rinfo : level_data.rivers_mutref()) {
    update_river(rinfo);
  }
}

void
game_loop(RenderArgs const& rargs, EngineState &es, ZoneManager &zm, SDLWindow &window,
    FrameTime const& ft)
{
  auto &logger = es.logger;
  auto &tiledata_state = es.tiledata_state;
  auto &zone_state = zm.active();

  move_betweentiledatas_ifonstairs(tiledata_state, zm);
  update_riverwiggles(zone_state.level_data, ft);

  /////////////////////////
  auto &leveldata = zone_state.level_data;

  auto &player = zone_state.player;
  auto &registry = zone_state.registry;
  /////////////////////////

  // compute tiledata
  if (tiledata_state.recompute) {
    LOG_INFO("Updating tiledata\n");

    update_visible_tiles(leveldata.tiledata_mutref(), player, tiledata_state.reveal);

    // We don't need to recompute the tiledata, we just did.
    tiledata_state.recompute = false;
  }

  // action begins here
  render::clear_screen(zone_state.background);

  if (es.draw_entities) {
    draw_entities(rargs, es, zone_state);
  }
  if (es.draw_terrain) {
    draw_terrain(rargs, zone_state);
  }
  if (tiledata_state.draw_tiledata) {
    draw_tiledata(rargs, zone_state, tiledata_state, ft);
    draw_rivers(rargs, zone_state, ft);
  }
  if (tiledata_state.show_grid_lines) {
    draw_tilegrid(rargs, zone_state, tiledata_state);
  }

  auto &sps = zone_state.sps;
  if (tiledata_state.show_neighbortile_arrows) {
    auto const& wp = player.world_position();
    auto const& tdata = leveldata.tiledata();
    auto const tpos = TilePosition::from_floats_truncated(wp.x, wp.z);
    draw_arrow_abovetile_and_neighbors(rargs, registry, sps, tdata, tpos);
  }
  if (es.show_global_axis) {
    draw_global_axis(rargs, registry, sps);
  }
  if (es.show_local_axis) {
    draw_local_axis(rargs, registry, sps, player.world_position());
  }
  if (es.show_player_localspace_vectors) {
    conditionally_draw_player_vectors(rargs, es, registry, sps, player);
  }

  // if checks happen inside fn
  conditionally_draw_player_vectors(rargs, es, registry, sps, player);
  if (es.ui_state.draw_ui) {
    draw_ui(es, zm, window, registry);
  }
}

} // ns boomhs

namespace
{

struct Engine
{
  SDLWindow window;
  std::vector<entt::DefaultRegistry> registries = {};

  Engine() = delete;
  explicit Engine(SDLWindow &&w)
    : window(MOVE(w))
  {
    registries.resize(50);
  }

  // We mark this as no-move/copy so the registries data never moves, allowing the rest of the
  // program to store references into the data owned by registries.
  NO_COPY_AND_NO_MOVE(Engine);

  auto dimensions() const { return window.get_dimensions(); }
};

void
loop(Engine &engine, GameState &state, FrameTime const& ft)
{
  auto &logger = state.engine_state.logger;
  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  SDL_Event event;
  boomhs::IO::process(state, event, ft);

  ZoneManager zm{state.zone_states};
  boomhs::game_loop(state.render_args(), state.engine_state, zm, engine.window, ft);

  // Render Imgui UI
  ImGui::Render();

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(engine.window.raw());
}

void
timed_game_loop(Engine &engine, GameState &state)
{
  window::Clock clock;
  window::FrameCounter counter;

  auto &logger = state.engine_state.logger;
  while (!state.engine_state.quit) {
    auto const ft = clock.frame_time();
    loop(engine, state, ft);
    clock.update(logger);
    counter.update(logger, clock);
  }
}

auto
make_init_gamestate(stlw::Logger &logger, ImGuiIO &imgui, window::Dimensions const &dimensions,
    ZoneStates &&zs)
{
  // Initialize opengl
  render::init(dimensions);

  // Configure Imgui
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  EngineState es{logger, imgui, dimensions};
  return GameState{MOVE(es), MOVE(zs)};
}

void
bridge_staircases(ZoneState &a, ZoneState &b)
{
  auto &tiledata_a = a.level_data.tiledata_mutref();
  auto &tiledata_b = b.level_data.tiledata_mutref();

  auto const stairs_up_a = find_upstairs(a.registry, tiledata_a);
  assert(!stairs_up_a.empty());

  //auto const stairs_down_a = find_downstairs(a.registry, a.tiledata);
  //assert(!stairs_down_a.empty());

  //auto const stairs_up_b = find_upstairs(b.registry, b.tiledata);
  //assert(!stairs_up_b.empty());

  auto const stairs_down_b = find_downstairs(b.registry, tiledata_b);
  assert(!stairs_down_b.empty());

  //std::cerr << "stairs_up_a: '" << stairs_up_a.size() << "'\n";
  //std::cerr << "stairs_down_a: '" << stairs_down_a.size() << "'\n";

  //std::cerr << "stairs_up_b: '" << stairs_up_b.size() << "'\n";
  //std::cerr << "stairs_down_b: '" << stairs_down_b.size() << "'\n";

  auto &a_registry = a.registry;
  auto &b_registry = b.registry;

  auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;

  assert(stairs_up_a.size() == stairs_down_b.size());
  auto const num_stairs = stairs_up_a.size();
  FOR(i, num_stairs) {
    auto const a_updowneid = stairs_up_a[i];
    auto const b_downeid = stairs_down_b[i];

    assert(a_registry.has<StairInfo>(a_updowneid));
    StairInfo &si_a = a_registry.get<StairInfo>(a_updowneid);

    assert(b_registry.has<StairInfo>(b_downeid));
    StairInfo &si_b = b_registry.get<StairInfo>(b_downeid);

    // find a suitable neighbor tile for each stair
    auto const a_neighbors = find_immediate_neighbors(tiledata_a, si_a.tile_position, TileType::FLOOR,
        behavior);
    assert(!a_neighbors.empty());

    auto const b_neighbors = find_immediate_neighbors(tiledata_b, si_b.tile_position, TileType::FLOOR,
        behavior);
    assert(!b_neighbors.empty());

    // Set A's exit position to B's neighbor, and visa versa
    si_a.exit_position = b_neighbors.front();
    si_b.exit_position = a_neighbors.front();
  }
}

stlw::result<stlw::empty_type, std::string>
start(stlw::Logger &logger, Engine &engine)
{
  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  // Construct tiledata
  stlw::float_generator rng;
  auto const make_zs = [&](int const floor_number, auto const floor_count, LevelAssets &&level_assets,
      entt::DefaultRegistry &registry)
  {
    auto &assets = level_assets.assets;
    auto const& objcache = assets.obj_cache;

    LOG_TRACE("Copy assets to GPU.");
    auto handle_result = boomhs::copy_assets_gpu(logger, level_assets.shader_programs, registry,
        objcache);
    assert(handle_result);
    HandleManager handlem = MOVE(*handle_result);

    auto const stairs_perfloor = 8;
    StairGenConfig const sgconfig{floor_count, floor_number, stairs_perfloor};

    int const width = 40, height = 40;
    TileDataConfig const tdconfig{width, height, sgconfig};
    auto leveldata = level_generator::make_leveldata(tdconfig, rng, registry);

    ////////////////////////////////
    // for now assume only 1 entity has the Light tag
    auto light_view = registry.view<PointLight, Transform>();
    for (auto const entity : light_view) {
      auto &transform = light_view.get<Transform>(entity);
      transform.scale = glm::vec3{0.2f};
    }

    // camera-look at origin
    // cameraspace "up" is === "up" in worldspace.
    auto const FORWARD = -Z_UNIT_VECTOR;
    auto constexpr UP = Y_UNIT_VECTOR;

    auto const player_eid = find_player(registry);

    EnttLookup player_lookup{player_eid, registry};
    WorldObject player{player_lookup, FORWARD, UP};
    Camera camera(player_lookup, FORWARD, UP);
    {
      SphericalCoordinates sc;
      sc.radius = 3.8f;
      sc.theta = glm::radians(-0.229f);
      sc.phi = glm::radians(38.2735f);
      camera.set_coordinates(MOVE(sc));
    }
    return ZoneState{
      assets.background_color,
      assets.global_light,
      MOVE(handlem),
      MOVE(level_assets.shader_programs),
      MOVE(level_assets.assets.texture_table),
      MOVE(leveldata),
      MOVE(camera),
      MOVE(player),
      registry};
  };

  auto &registries = engine.registries;
  auto const FLOOR_COUNT = 5;

  std::vector<ZoneState> zstates;
  FOR(i, 5) {
    auto const level_string = [&i]() { return "area" + std::to_string(i) + ".toml"; };
    DO_TRY(auto ld, boomhs::load_level(logger, registries[i], level_string()));
    auto zs = make_zs(i, FLOOR_COUNT, MOVE(ld), registries[i]);
    zstates.emplace_back(MOVE(zs));

    if (i == 0) {
      continue;
    }
    bridge_staircases(zstates[i-1], zstates[i]);
  }
  ZoneStates zs{MOVE(zstates)};

  auto &imgui = ImGui::GetIO();
  auto state = make_init_gamestate(logger, imgui, engine.dimensions(), MOVE(zs));
  timed_game_loop(engine, state);
  LOG_TRACE("game loop finished.");
  return stlw::empty_type{};
}

} // ns anon

using WindowResult = stlw::result<SDLWindow, std::string>;
WindowResult
make_window(Logger &logger, bool const fullscreen, float const width, float const height)
{
  // Select windowing library as SDL.
  LOG_DEBUG("Initializing window library globals");
  DO_TRY(auto _, window::sdl_library::init());

  LOG_DEBUG("Instantiating window instance.");
  return window::sdl_library::make_window(fullscreen, height, width);
}

int
main(int argc, char *argv[])
{
  Logger logger = stlw::log_factory::make_default_logger("main logger");
  auto const on_error = [&logger](auto const &error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  LOG_DEBUG("Creating window ...");
  bool constexpr FULLSCREEN = false;
  DO_TRY_OR_ELSE_RETURN(auto window, make_window(logger, FULLSCREEN, 1024, 768),
                        on_error);
  Engine engine{MOVE(window)};

  LOG_DEBUG("Starting game loop");
  DO_TRY_OR_ELSE_RETURN(auto _, start(logger, engine), on_error);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
