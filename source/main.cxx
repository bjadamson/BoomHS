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
copy_assets_gpu(stlw::Logger &logger, opengl::ShaderPrograms &sps, entt::DefaultRegistry &registry,
                ObjCache const &obj_cache)
{
  GpuHandleList handle_list;
  /*
  registry.view<ShaderName, opengl::Color, CubeRenderable>().each(
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

  registry.view<ShaderName, opengl::Color, MeshRenderable>().each(
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

  auto const plus_eid = make_special("plus", "3d_pos_normal_color");
  auto const hashtag_eid = make_special("hashtag", "hashtag");
  auto const river_eid = make_special("tilde", "river");
  auto const stair_down_eid = make_stair("stair", "stair_down");
  auto const stair_up_eid = make_stair("stair", "stair_up");

  return HandleManager{MOVE(handle_list), plus_eid, hashtag_eid, river_eid, stair_down_eid,
    stair_up_eid};
}

void
draw_entities(GameState &state, opengl::ShaderPrograms &sps)
{
  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &handlem = active.handles;
  auto &registry = active.registry;

  auto const draw_fn = [&handlem, &sps, &registry, &state](auto entity, auto &sn, auto &transform) {
    auto &shader_ref = sps.ref_sp(sn.value);
    auto &handle = handlem.lookup(entity);
    render::draw(state.render_args(), transform, shader_ref, handle, entity, registry);
  };

  auto const draw_adapter = [&](auto entity, auto &sn, auto &transform, auto &) {
    draw_fn(entity, sn, transform);
  };

  //
  // Actual drawing begins here
  registry.view<ShaderName, Transform, CubeRenderable>().each(draw_adapter);
  registry.view<ShaderName, Transform, MeshRenderable>().each(draw_adapter);

  if (state.engine_state.draw_skybox) {
    auto const draw_skybox = [&](auto entity, auto &sn, auto &transform, auto &) {
      draw_fn(entity, sn, transform);
    };
    registry.view<ShaderName, Transform, SkyboxRenderable>().each(draw_skybox);
  }
}

void
draw_terrain(GameState &state, opengl::ShaderPrograms &sps)
{
  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &registry = active.registry;

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &logger = state.engine_state.logger;
  auto &terrain_sp = sps.ref_sp("3d_pos_normal_color");
  auto terrain = OF::copy_normalcolorcube_gpu(logger, terrain_sp, LOC::WHITE);

  auto &transform = registry.assign<Transform>(entity);
  auto &scale = transform.scale;
  scale.x = 50.0f;
  scale.y = 0.2f;
  scale.z = 50.0f;
  auto &translation = transform.translation;
  // translation.x = 3.0f;
  translation.y = -2.0f;
  // translation.z = 2.0f;
  render::draw(state.render_args(), transform, terrain_sp, terrain, entity, registry);
}

void
draw_tiledata(GameState &state, opengl::ShaderPrograms &sps, FrameTime const& ft)
{
  using namespace render;
  auto &logger = state.engine_state.logger;
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

  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &handlem = active.handles;

  DrawPlusArgs plus{sps.ref_sp("3d_pos_normal_color"), handlem.lookup(handlem.plus_eid),
    handlem.plus_eid};
  DrawHashtagArgs hashtag{sps.ref_sp("hashtag"), handlem.lookup(handlem.hashtag_eid),
                          handlem.hashtag_eid};
  DrawHashtagArgs river{sps.ref_sp("river"), handlem.lookup(handlem.river_eid),
                          handlem.river_eid};

  auto &stair_sp = sps.ref_sp("stair");
  DrawStairsDownArgs stairs_down{stair_sp, handlem.lookup(handlem.stair_down_eid), handlem.stair_down_eid};
  DrawStairsUpArgs stairs_up{stair_sp, handlem.lookup(handlem.stair_up_eid), handlem.stair_up_eid};
  DrawTileDataArgs dta{MOVE(plus), MOVE(hashtag), MOVE(river), MOVE(stairs_down), MOVE(stairs_up)};

  auto &registry = active.registry;
  auto const& leveldata = active.level_data;
  render::draw_tiledata(state.render_args(), dta, leveldata.tiledata(),
                       state.engine_state.tiledata_state, registry, ft);
}

void
draw_rivers(GameState &state, opengl::ShaderPrograms &sps, FrameTime const& ft)
{
  auto const rargs = state.render_args();
  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &registry = active.registry;
  auto &handlem = active.handles;

  auto &sp = sps.ref_sp("river");
  auto &river_handle = handlem.lookup(handlem.river_eid);

  auto const& rinfos = active.level_data.rivers();
  for (auto const& rinfo : rinfos) {
    render::draw_rivers(rargs, sp, river_handle, registry, ft, handlem.river_eid, rinfo);
  }
}

void
draw_tilegrid(GameState &state, opengl::ShaderPrograms &sps)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &sp = sps.ref_sp("3d_pos_color");

  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &registry = active.registry;
  auto const& leveldata = active.level_data;
  auto const& tiledata = leveldata.tiledata();

  Transform transform;
  bool const show_y = es.tiledata_state.show_yaxis_lines;
  auto const tilegrid = OF::create_tilegrid(logger, sp, tiledata, show_y);
  render::draw_tilegrid(state.render_args(), transform, sp, tilegrid);
}

void
draw_global_axis(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto world_arrows = OF::create_axis_arrows(logger, sp);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);

  auto const &rargs = state.render_args();
  render::draw(rargs, transform, sp, world_arrows.x_dinfo, entity, registry);
  render::draw(rargs, transform, sp, world_arrows.y_dinfo, entity, registry);
  render::draw(rargs, transform, sp, world_arrows.z_dinfo, entity, registry);
}

void
draw_local_axis(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
                glm::vec3 const &player_pos)
{
  auto &logger = state.engine_state.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto const axis_arrows = OF::create_axis_arrows(logger, sp);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);
  transform.translation = player_pos;

  auto const &rargs = state.render_args();
  render::draw(rargs, transform, sp, axis_arrows.x_dinfo, entity, registry);
  render::draw(rargs, transform, sp, axis_arrows.y_dinfo, entity, registry);
  render::draw(rargs, transform, sp, axis_arrows.z_dinfo, entity, registry);
}

void
draw_arrow(GameState &state, opengl::ShaderPrograms &sps, entt::DefaultRegistry &registry,
    glm::vec3 const& start, glm::vec3 const& head, Color const& color)
{
  auto &sp = sps.ref_sp("3d_pos_color");
  auto &logger = state.engine_state.logger;

  auto const handle = OF::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });
  auto &transform = registry.assign<Transform>(entity);

  auto const &rargs = state.render_args();
  render::draw(rargs, transform, sp, handle, entity, registry);
}

void
draw_arrow_abovetile_and_neighbors(GameState &state, entt::DefaultRegistry &registry,
    ShaderPrograms &sps, TileData const& tdata, TilePosition const& tpos)
{
  glm::vec3 constexpr offset{0.5f, 2.0f, 0.5f};

  auto const draw_the_arrow = [&](auto const& ntpos, auto const& color) {
    auto const bottom = glm::vec3{ntpos.x + offset.x, offset.y, ntpos.y + offset.y};
    auto const top = bottom + (Y_UNIT_VECTOR * 2.0f);

    draw_arrow(state, sps, registry, top, bottom, color);
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
conditionally_draw_player_vectors(GameState &state, entt::DefaultRegistry &registry, ShaderPrograms &sps,
    WorldObject const &player)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;

  glm::vec3 const pos = player.world_position();
  if (es.show_player_localspace_vectors) {
    // local-space
    //
    // forward
    auto const fwd = player.eye_forward();
    draw_arrow(state, sps, registry, pos, pos + fwd, LOC::GREEN);

    // right
    auto const right = player.eye_right();
    draw_arrow(state, sps, registry, pos, pos + right, LOC::RED);
  }
  if (es.show_player_worldspace_vectors) {
    // world-space
    //
    // forward
    auto const fwd = player.world_forward();
    draw_arrow(state, sps, registry, pos, pos + (2.0f * fwd), LOC::LIGHT_BLUE);

    // backward
    glm::vec3 const right = player.world_right();
    draw_arrow(state, sps, registry, pos, pos + right, LOC::PINK);
  }
}

void
move_betweentiledatas_ifonstairs(GameState &state)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &tiledata_state = es.tiledata_state;

  ZoneManager zm{state.zone_states};
  auto &zone_state = zm.active();

  auto &player = zone_state.player;
  auto &registry = zone_state.registry;

  auto const wp_orzeroifnan = [](auto const& wp) {
    auto const anynan = stlw::math::anynan(wp);
    auto const allnan = stlw::math::allnan(wp);

    // If any are NaN, then all should be NaN.
    assert(anynan ? allnan : true);
    if (anynan) {
      return glm::zero<glm::vec3>();
    }
    return wp;
  };

  auto const eid = find_player(registry);
  auto &pc = registry.get<Player>(eid);
  registry.get<Transform>(eid).translation.y = 0.5f;

  auto const cast = [](float const f) { return static_cast<int>(f);};
  auto &tp = pc.tile_position;

  auto const& leveldata = zone_state.level_data;
  auto const& tiledata = leveldata.tiledata();

  auto const wp = player.world_position();
  {
    auto const [w, h] = leveldata.dimensions();
    assert(wp.x < w);
    assert(wp.y < h);
  }
  if (tp == TilePosition{cast(wp.x), cast(wp.z)}) {
    return;
  }
  tp.x = cast(wp.x);
  tp.y = cast(wp.z);

  auto const& tile = tiledata.data(tp);
  if (!tile.is_stair()) {
    return;
  }
  // lookup stairs in tiledata
  auto const stair_eids = find_stairs(registry);
  assert(!stair_eids.empty());

  auto const move_player_through_stairs = [&state, &tile, &tiledata_state](StairInfo const& stair) {
    {
      ZoneManager zm{state.zone_states};
      auto &zone_state = zm.active();

      int const current = zm.active_zone();
      int const newlevel = current + (tile.is_stair_up() ? 1 : -1);
      //std::cerr << "moving through stair '" << stair.direction << "'\n";
      assert(newlevel < zm.num_zones());
      zm.make_zone_active(newlevel, state);
    }

    // now that the zone has changed, all references through zm are pointing to old level
    ZoneManager zm{state.zone_states};
    auto &zone_state = zm.active();

    auto &camera = zone_state.camera;
    auto &player = zone_state.player;
    auto &registry = zone_state.registry;

    auto const spos = stair.exit_position;
    player.move_to(spos.x, player.world_position().y, spos.y);
    player.rotate_to_match_camera_rotation(camera);

    tiledata_state.recompute = true;
  };
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
update_riverwiggles(GameState &state, FrameTime const& ft)
{
  auto const update_river = [&ft](auto &rinfo)
  {
    auto const& left = rinfo.left;
    auto const& right = rinfo.right;

    for (auto &wiggle : rinfo.wiggles) {
      auto &x = wiggle.position.x;

      x += std::abs(std::cos(ft.delta)) / wiggle.speed;
      if (x > right.x) {
        x = left.x;
      }
    }
  };

  ZoneManager zm{state.zone_states};
  auto &level_data = zm.active().level_data;

  for (auto &rinfo : level_data.rivers_mutref()) {
    update_river(rinfo);
  }
}

void
game_loop(GameState &state, SDLWindow &window, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &tiledata_state = es.tiledata_state;
  move_betweentiledatas_ifonstairs(state);
  update_riverwiggles(state, ft);

  /////////////////////////
  ZoneManager zm{state.zone_states};
  auto &zone_state = zm.active();
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

  auto &sps = zone_state.sps;
  if (es.draw_entities) {
    draw_entities(state, sps);
  }
  if (es.draw_terrain) {
    draw_terrain(state, sps);
  }
  if (tiledata_state.draw_tiledata) {
    draw_tiledata(state, sps, ft);
    draw_rivers(state, sps, ft);
  }
  if (tiledata_state.show_grid_lines) {
    draw_tilegrid(state, sps);
  }
  if (tiledata_state.show_neighbortile_arrows) {
    auto const& wp = player.world_position();
    auto const& tdata = leveldata.tiledata();
    auto const tpos = TilePosition{static_cast<int>(wp.x), static_cast<int>(wp.z)};
    draw_arrow_abovetile_and_neighbors(state, registry, sps, tdata, tpos);
  }
  if (es.show_global_axis) {
    draw_global_axis(state, registry, sps);
  }
  if (es.show_local_axis) {
    draw_local_axis(state, registry, sps, player.world_position());
  }
  if (es.show_player_localspace_vectors) {
    conditionally_draw_player_vectors(state, registry, sps, player);
  }

  // if checks happen inside fn
  conditionally_draw_player_vectors(state, registry, sps, player);
  if (es.ui_state.draw_ui) {
    draw_ui(state, window, registry);
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
    registries.resize(5);
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
  boomhs::game_loop(state, engine.window, ft);

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
    auto const& assets = level_assets.assets;
    auto const& objcache = assets.obj_cache;

    auto &sps = level_assets.shader_programs;

    LOG_TRACE("Copy assets to GPU.");
    auto handle_result = boomhs::copy_assets_gpu(logger, sps, registry, objcache);
    assert(handle_result);
    HandleManager handlem = MOVE(*handle_result);

    auto const stairs_perfloor = 8;
    StairGenConfig const sgconfig{floor_count, floor_number, stairs_perfloor};

    int const width = 40, length = 40;
    TilemapConfig tconfig{width, length, sgconfig};

    auto genlevel = level_generator::make_tiledata(tconfig, rng, registry);
    auto tdata = MOVE(genlevel.first);
    auto startpos = MOVE(genlevel.second);

    LevelData ldata{MOVE(tdata), MOVE(startpos)};

    ////////////////////////////////
    // for now assume only 1 entity has the Light tag
    auto light_view = registry.view<PointLight, Transform>();
    for (auto const entity : light_view) {
      auto &transform = light_view.get<Transform>(entity);
      transform.scale = glm::vec3{0.2f};
    }

    // camera-look at origin
    // cameraspace "up" is === "up" in worldspace.
    auto const FORWARD = -opengl::Z_UNIT_VECTOR;
    auto constexpr UP = opengl::Y_UNIT_VECTOR;

    auto const player_eid = find_player(registry);

    EnttLookup player_lookup{player_eid, registry};
    WorldObject player{player_lookup, FORWARD, UP};

    Camera camera(player_lookup, FORWARD, UP);

    SphericalCoordinates sc;
    sc.radius = 3.8f;
    sc.theta = glm::radians(-0.229f);
    sc.phi = glm::radians(38.2735f);
    camera.set_coordinates(MOVE(sc));
    //////////////////////////

    return ZoneState{assets.background_color, assets.global_light, MOVE(handlem), MOVE(sps),
      MOVE(ldata), MOVE(camera), MOVE(player), registry};
  };

  auto &registries = engine.registries;
  auto const FLOOR_COUNT = 5;

  std::vector<ZoneState> zstates;


  DO_TRY(auto ld0, boomhs::load_level(logger, registries[0], "area0.toml"));
  DO_TRY(auto ld1, boomhs::load_level(logger, registries[1], "area1.toml"));
  DO_TRY(auto ld2, boomhs::load_level(logger, registries[2], "area2.toml"));
  DO_TRY(auto ld3, boomhs::load_level(logger, registries[3], "area3.toml"));
  DO_TRY(auto ld4, boomhs::load_level(logger, registries[4], "area4.toml"));

  auto zs0 = make_zs(0, FLOOR_COUNT, MOVE(ld0), registries[0]);
  auto zs1 = make_zs(1, FLOOR_COUNT, MOVE(ld1), registries[1]);
  auto zs2 = make_zs(2, FLOOR_COUNT, MOVE(ld2), registries[2]);
  auto zs3 = make_zs(3, FLOOR_COUNT, MOVE(ld3), registries[3]);
  auto zs4 = make_zs(4, FLOOR_COUNT, MOVE(ld4), registries[4]);

  //FOR(i, 5) {
    //DO_TRY(auto ld, boomhs::load_level(logger, registries[i], "area" + std::to_string(i) + ".toml"));
    //auto zs = make_zs(i, FLOOR_COUNT, MOVE(ld), registries[i]);
    //zstates.emplace_back(MOVE(zs));

    //bridge_staircases(zstates[i-1], zstates[i]);
  //}

  zstates.emplace_back(MOVE(zs0));
  zstates.emplace_back(MOVE(zs1));
  zstates.emplace_back(MOVE(zs2));
  zstates.emplace_back(MOVE(zs3));
  zstates.emplace_back(MOVE(zs4));

  bridge_staircases(zstates[0], zstates[1]);
  bridge_staircases(zstates[1], zstates[2]);
  bridge_staircases(zstates[2], zstates[3]);
  bridge_staircases(zstates[3], zstates[4]);

  ZoneStates zs{MOVE(zstates)};

  auto &imgui = ImGui::GetIO();
  auto state = make_init_gamestate(logger, imgui, engine.dimensions(), MOVE(zs));

  /*
  auto &zstates = state.zone_states;
  ZoneManager zm{zstates};

  registries.resize(FLOOR_COUNT);
  FOR(i, FLOOR_COUNT) {
    DO_TRY(auto ld, boomhs::load_level(logger, registries[i], "area" + std::to_string(i) + ".toml"));
    auto zs = make_zs(i, FLOOR_COUNT, MOVE(ld), registries[i]);
    zm.add_zone(MOVE(zs));
  }
  assert(FLOOR_COUNT >= 1);
  for(auto i = 1; i < FLOOR_COUNT; ++i) {
    bridge_staircases(zstates[i - 1], zstates[i]);
  }
  */

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
