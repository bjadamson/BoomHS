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

  auto const make_special = [&handle_list, &logger, &obj_cache, &sps, &registry](char const *name) {
    auto const &obj = obj_cache.get_obj(name);
    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, sps.ref_sp(name), obj, boost::none);
    auto const entity = registry.create();
    registry.assign<Transform>(entity);//.scale = glm::vec3{0.2f, 0.2f, 0.2f};
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

  auto const plus_eid = make_special("plus");
  auto const hashtag_eid = make_special("hashtag");

  return HandleManager{MOVE(handle_list), plus_eid, hashtag_eid};
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
  DrawTilemapArgs dta{MOVE(plus), MOVE(hashtag)};

  ZoneManager zm{state.zone_states};
  auto const& tilemap = zm.active().tilemap;
  render::draw_tilemap(state.render_args(), dta, tilemap,
                       state.engine_state.tilemap_state, registry);
}

void
draw_tilegrid(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps)
{
  auto &engine_state = state.engine_state;
  auto &logger = engine_state.logger;
  auto &sp = sps.ref_sp("3d_pos_color");

  ZoneManager zm{state.zone_states};
  auto const& tilemap = zm.active().tilemap;

  Transform transform;
  bool const show_y = engine_state.tilemap_state.show_yaxis_lines;
  auto const tilegrid = OF::create_tilegrid(logger, sp, tilemap, show_y);
  render::draw_tilegrid(state.render_args(), transform, sp, tilegrid);
}

void
draw_global_axis(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps)
{
  auto &engine_state = state.engine_state;
  auto &logger = engine_state.logger;
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
draw_target_vectors(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
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

  // draw player forward
  {
    glm::vec3 const player_pos = player.world_position();
    glm::vec3 const player_fwd = player_pos + (2.0f * player.forward_vector());

    draw_arrow(player_pos, player_fwd, LOC::LIGHT_BLUE);
  }

  // draw forward arrow (for camera) ??
  {
    glm::vec3 const start = glm::vec3{0, 0, 0};
    glm::vec3 const head = state.engine_state.ui_state.last_mouse_clicked_pos;
    draw_arrow(start, head, LOC::PURPLE);
  }

  // draw forward arrow (for camera) ??
  {
    glm::vec3 const start = player.world_position();
    glm::vec3 const head = start + player.backward_vector();
    draw_arrow(start, head, LOC::PINK);
  };
}

void
game_loop(GameState &state, window::SDLWindow &window, double const dt)
{
  auto &engine_state = state.engine_state;
  ZoneManager zm{state.zone_states};
  auto &zone_state = zm.active();

  auto &player = zone_state.player;
  auto &mouse = engine_state.mouse_state;
  auto &tilemap_state = state.engine_state.tilemap_state;
  auto &logger = engine_state.logger;
  auto &registry = zone_state.registry;

  if (mouse.both_pressed()) {
    // We do this here, so that it gets checked every frame.
    //
    // While the buttons are pressed, move the player.
    move_ontilemap(state, &WorldObject::forward_vector, player, dt);
  }

  // compute tilemap
  if (tilemap_state.recompute) {
    LOG_INFO("Updating tilemap\n");

    update_visible_tiles(zone_state.tilemap, player, tilemap_state.reveal);

    // We don't need to recompute the tilemap, we just did.
    tilemap_state.recompute = false;
  }

  // action begins here
  render::clear_screen(zone_state.background);

  auto &handles = zone_state.handles;
  auto &sps = zone_state.sps;
  if (engine_state.draw_entities) {
    draw_entities(state, registry, sps, handles);
  }
  if (engine_state.draw_terrain) {
    draw_terrain(state, registry, sps);
  }
  if (tilemap_state.draw_tilemap) {
    draw_tilemap(state, registry, sps, handles);
  }
  if (tilemap_state.show_grid_lines) {
    draw_tilegrid(state, registry, sps);
  }
  if (engine_state.show_global_axis) {
    draw_global_axis(state, registry, sps);
  }
  if (engine_state.show_local_axis) {
    draw_local_axis(state, registry, sps, player.world_position());
  }
  if (engine_state.show_target_vectors) {
    draw_target_vectors(state, registry, sps, player);
  }
  if (engine_state.ui_state.draw_ui) {
    draw_ui(state, window, registry);
  }
}

} // ns boomhs

namespace
{

struct Engine
{
  ::window::SDLWindow window;
  std::array<entt::DefaultRegistry, 2> registries = {};

  MOVE_CONSTRUCTIBLE_ONLY(Engine);

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
  auto const make_zs = [&](LevelData &&level_data, entt::DefaultRegistry &registry) {
    LOG_TRACE("Copy assets to GPU.");
    auto const& objcache = level_data.assets.obj_cache;
    auto &sps = level_data.shader_programs;

    auto handle_result = boomhs::copy_assets_gpu(logger, sps, registry, objcache);
    assert(handle_result);
    auto handlem = MOVE(*handle_result);

    auto tmap_startingpos = level_generator::make_tilemap(35, 1, 35, rng);
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
    auto &player_transform = registry.get<Transform>(player_eid);
    //player_transform.rotation = glm::angleAxis(glm::radians(180.0f), opengl::Y_UNIT_VECTOR);

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
  DO_TRY(auto ld0, boomhs::load_level(logger, registries[0], "area0.toml"));
  DO_TRY(auto ld1, boomhs::load_level(logger, registries[1], "area0.toml"));
  auto zs0 = make_zs(MOVE(ld0), registries[0]);
  auto zs1 = make_zs(MOVE(ld1), registries[1]);

  std::array<ZoneState, ZoneStates::NUM_ZSTATES> zstates_arr{MOVE(zs0), MOVE(zs1)};
  ZoneStates zs{MOVE(zstates_arr)};

  auto &imgui = ImGui::GetIO();
  auto state = make_init_gamestate(logger, imgui, MOVE(zs), engine.dimensions());

  timed_game_loop(engine, state);
  LOG_TRACE("game loop finished.");
  return stlw::empty_type{};
}

} // ns anon

using EngineResult = stlw::result<Engine, std::string>;
using stlw::Logger;

EngineResult
make_opengl_sdl_engine(Logger &logger, bool const fullscreen, float const width, float const height)
{
  // Select windowing library as SDL.
  LOG_DEBUG("Initializing window library globals");
  DO_TRY(auto _, window::sdl_library::init());

  LOG_DEBUG("Instantiating window instance.");
  DO_TRY(auto window, window::sdl_library::make_window(fullscreen, height, width));

  return Engine{MOVE(window)};
}

int
main(int argc, char *argv[])
{
  Logger logger = stlw::log_factory::make_default_logger("main logger");
  auto const on_error = [&logger](auto const &error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  bool constexpr FULLSCREEN = false;
  DO_TRY_OR_ELSE_RETURN(auto engine, make_opengl_sdl_engine(logger, FULLSCREEN, 1024, 768),
                        on_error);

  LOG_DEBUG("Starting game loop");
  DO_TRY_OR_ELSE_RETURN(auto _, start(logger, engine), on_error);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
