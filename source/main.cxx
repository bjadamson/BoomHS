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
#include <boomhs/tilemap.hpp>
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
#include <boost/optional.hpp>

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
        auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, boost::none);
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
    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, boost::none);
    handle_list.add(entity, MOVE(handle));
  });

  auto const make_special = [&handle_list, &logger, &obj_cache, &sps, &registry](char const *name, char const*vshader_name) {
    auto const &obj = obj_cache.get_obj(name);
    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, sps.ref_sp(vshader_name), obj, boost::none);
    auto const entity = registry.create();
    registry.assign<Transform>(entity);
    registry.assign<Material>(entity);
    auto meshc = registry.assign<MeshRenderable>(entity);
    meshc.name = name;

    auto &color = registry.assign<Color>(entity);
    color.set_r(1.0);
    color.set_g(1.0);
    color.set_b(1.0);
    color.set_a(1.0);

    handle_list.add(entity, MOVE(handle));
    return entity;
  };

  auto const plus_eid = make_special("plus", "plus");
  auto const hashtag_eid = make_special("hashtag", "hashtag");
  auto const stairs_eid = make_special("stairs", "3d_pos_normal_color");

  return HandleManager{MOVE(handle_list), plus_eid, hashtag_eid, stairs_eid};
}

void
draw_entities(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
              HandleManager &handles)
{
  auto const draw_fn = [&handles, &sps, &registry, &state](auto entity, auto &sn, auto &transform) {
    auto &shader_ref = sps.ref_sp(sn.value);
    auto &handle = handles.lookup(entity);
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
draw_terrain(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps)
{
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
draw_tilemap(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
             HandleManager &handles)
{
  using namespace render;
  auto &logger = state.engine_state.logger;
  // TODO: How do we "GET" the hashtag and plus shaders under the new entity management system?
  //
  // PROBLEM:
  //    We currently store one draw handle to every entity. For the TileMap, we need to store two
  //    (an array) of DrawInfo instances to the one entity.
  //
  // THOUGHT:
  //    It probably makes sense to render the tilemap differently now. We should assume more than
  //    just two tile type's will be necessary, and will have to devise a strategy for quickly
  //    rendering the tilemap using different tile's. Maybe store the different tile type's
  //    together somehow for rendering?
  DrawPlusArgs plus{sps.ref_sp("plus"), handles.lookup(handles.plus_eid), handles.plus_eid};
  DrawHashtagArgs hashtag{sps.ref_sp("hashtag"), handles.lookup(handles.hashtag_eid),
                          handles.hashtag_eid};
  DrawStairsArgs stairs{sps.ref_sp("3d_pos_normal_color"), handles.lookup(handles.stairs_eid),
    handles.stairs_eid};
  DrawTilemapArgs dta{MOVE(plus), MOVE(hashtag), MOVE(stairs)};

  ZoneManager zm{state.zone_states};
  auto const& tilemap = zm.active().tilemap;
  render::draw_tilemap(state.render_args(), dta, tilemap,
                       state.engine_state.tilemap_state, registry);
}

void
draw_tilegrid(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &sp = sps.ref_sp("3d_pos_color");

  ZoneManager zm{state.zone_states};
  auto const& tilemap = zm.active().tilemap;

  Transform transform;
  bool const show_y = es.tilemap_state.show_yaxis_lines;
  auto const tilegrid = OF::create_tilegrid(logger, sp, tilemap, show_y);
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
  transform.translation = glm::vec3{0.0, 0.0, 0.0}; // explicit

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
conditionally_draw_player_vectors(GameState &state, entt::DefaultRegistry &registry, ShaderPrograms &sps,
    WorldObject const &player)
{
  auto &logger = state.engine_state.logger;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto const draw_arrow = [&](auto const &start, auto const &head, auto const &color) {
    auto const handle = OF::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});

    auto entity = registry.create();
    ON_SCOPE_EXIT([&]() { registry.destroy(entity); });
    auto &transform = registry.assign<Transform>(entity);

    auto const &rargs = state.render_args();
    render::draw(rargs, transform, sp, handle, entity, registry);
  };

  auto &es = state.engine_state;
  glm::vec3 const pos = player.world_position();
  if (es.show_player_localspace_vectors) {
    // local-space
    //
    // forward
    auto const fwd = player.eye_forward();
    draw_arrow(pos, pos + fwd, LOC::GREEN);

    // right
    auto const right = player.eye_right();
    draw_arrow(pos, pos + right, LOC::RED);
  }
  if (es.show_player_worldspace_vectors) {
    // world-space
    //
    // forward
    auto const fwd = player.world_forward();
    draw_arrow(pos, pos + (2.0f * fwd), LOC::LIGHT_BLUE);

    // backward
    glm::vec3 const right = player.world_right();
    draw_arrow(pos, pos + right, LOC::PINK);
  }
}

void
move_betweentilemaps_ifonstairs(GameState &state)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &tilemap_state = es.tilemap_state;

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

  // We need to check for NaN in cases where the camera becomes perpendicular to the Y axis. When
  // this occurs the view calculation (lookAt) can yield Nan's for the position.
  //
  // Ran into this during development, thus handling it now.
  auto const wp = player.world_position();
  std::cerr << "wp.x '" << wp.x << " wp.z '" << wp.z << "'\n";

  auto const eid = find_player(registry);
  auto &pc = registry.get<Player>(eid);
  auto const cast = [](float const f) { return static_cast<int>(f);};
  auto &tp = pc.tile_position;
  auto &tmap = zone_state.tilemap;
  {
    auto const [w, l] = tmap.dimensions();
    // TODO: work out why x/z are NaN after moving levels.
    assert(wp.x < w);
    assert(wp.z < l);
  }
  if (tp == TilePosition{cast(wp.x), cast(wp.z)}) {
    return;
  }
  tp.x = cast(wp.x);
  tp.z = cast(wp.z);

  auto const& tile = tmap.data(tp);
  if (tile.type != TileType::STAIRS) {
    return;
  }
  // lookup stairs in tilemap
  auto const stair_eids = find_stairs(registry);
  assert(!stair_eids.empty());

  auto const move_player_through_stairs = [&](StairInfo const& stair) {
    int const current = zm.active_zone();
    int const newlevel = current + (stair.direction == StairDirection::UP ? 1 : -1);
    std::cerr << "moving through stair '" << stair.direction << "'\n";
    assert(newlevel < zm.num_zones());
    zm.make_zone_active(newlevel, state);
    player.move_to(stair.exit_position);
    tilemap_state.recompute = true;
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
game_loop(GameState &state, SDLWindow &window, double const dt)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &tilemap_state = es.tilemap_state;
  move_betweentilemaps_ifonstairs(state);

  /////////////////////////
  ZoneManager zm{state.zone_states};
  auto &zone_state = zm.active();
  auto &tmap = zone_state.tilemap;

  auto &player = zone_state.player;
  auto &registry = zone_state.registry;

  auto const wp = player.world_position();
  /////////////////////////

  // compute tilemap
  if (tilemap_state.recompute) {
    LOG_INFO("Updating tilemap\n");

    update_visible_tiles(tmap, player, tilemap_state.reveal);

    // We don't need to recompute the tilemap, we just did.
    tilemap_state.recompute = false;
  }

  // action begins here
  render::clear_screen(zone_state.background);

  auto &handles = zone_state.handles;
  auto &sps = zone_state.sps;
  if (es.draw_entities) {
    draw_entities(state, registry, sps, handles);
  }
  if (es.draw_terrain) {
    draw_terrain(state, registry, sps);
  }
  if (tilemap_state.draw_tilemap) {
    draw_tilemap(state, registry, sps, handles);
  }
  if (tilemap_state.show_grid_lines) {
    draw_tilegrid(state, registry, sps);
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
loop(Engine &engine, GameState &state, double const dt)
{
  auto &logger = state.engine_state.logger;
  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  SDL_Event event;
  boomhs::IO::process(state, event, dt);
  boomhs::game_loop(state, engine.window, dt);

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
  auto const freq = SDL_GetPerformanceFrequency();
  while (!state.engine_state.quit) {
    double const dt = (clock.ticks() * 1000.0 / freq);

    loop(engine, state, dt);
    clock.update(logger);
    counter.update(logger, clock);
  }
}

auto
make_init_gamestate(stlw::Logger &logger, ImGuiIO &imgui, ZoneStates &&zs, window::Dimensions const &dimensions)
{
  // Initialize opengl
  render::init(dimensions);

  // Configure Imgui
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  EngineState es{logger, imgui, dimensions};
  return GameState{MOVE(es), MOVE(zs)};
}

stlw::result<stlw::empty_type, std::string>
start(stlw::Logger &logger, Engine &engine)
{
  using namespace opengl;

  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  // Construct tilemap
  stlw::float_generator rng;
  auto const make_zs = [&](int const floor_number, auto const floor_count, LevelData &&level_data,
      entt::DefaultRegistry &registry)
  {
    auto const& objcache = level_data.assets.obj_cache;
    auto &sps = level_data.shader_programs;

    LOG_TRACE("Copy assets to GPU.");
    auto handle_result = boomhs::copy_assets_gpu(logger, sps, registry, objcache);
    assert(handle_result);
    auto handlem = MOVE(*handle_result);

    auto const stairs_perfloor = 2;
    StairGenConfig const sgconfig{floor_count, floor_number, stairs_perfloor};

    int const width = 40, length = 40;
    TilemapConfig tconfig{width, length, sgconfig};

    auto tmap_startingpos = level_generator::make_tilemap(tconfig, rng, registry);
    auto tmap = MOVE(tmap_startingpos.first);
    auto &startingpos = tmap_startingpos.second;

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
    camera.set_coordinates(MOVE(sc));
    //////////////////////////

    auto const& assets = level_data.assets;
    return ZoneState{assets.background_color, assets.global_light, MOVE(handlem), MOVE(sps),
      MOVE(tmap), MOVE(camera), MOVE(player), registry};
  };

  auto &registries = engine.registries;
  auto const FLOOR_COUNT = 5;

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

  std::array<ZoneState, FLOOR_COUNT> zstates_arr{MOVE(zs0), MOVE(zs1), MOVE(zs2),
   MOVE(zs3), MOVE(zs4)};
  ZoneStates zs{MOVE(zstates_arr)};

  /*
  auto &registries = engine.registries;
  static auto constexpr FLOOR_COUNT = 2;
  std::vector<ZoneState> zstates;
  //registries.resize(FLOOR_COUNT);

  FOR(i, FLOOR_COUNT) {
    DO_TRY(auto ld, boomhs::load_level(logger, registries[i], "area0.toml"));
    auto zs = make_zs(i, FLOOR_COUNT, MOVE(ld), registries[i]);
    zstates.emplace_back(MOVE(zs));
  }
  ZoneStates zs{MOVE(zstates)};
  */

  auto &imgui = ImGui::GetIO();
  auto state = make_init_gamestate(logger, imgui, MOVE(zs), engine.dimensions());

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
