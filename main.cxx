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

#include <stlw/log.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>

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
using stlw::Logger;

namespace boomhs
{

stlw::result<HandleManager, std::string>
copy_assets_gpu(stlw::Logger &logger, opengl::ShaderPrograms &sps, entt::DefaultRegistry &registry,
                ObjCache const &obj_cache)
{
  GpuHandleList handle_list;
  registry.view<ShaderName, opengl::Color, CubeRenderable>().each(
      [&](auto entity, auto &sn, auto &color, auto &) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_colorcube_gpu(logger, shader_ref, color);
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

  return HandleManager{MOVE(handle_list)};
}

auto&
find_player(entt::DefaultRegistry &registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Player>().size());

  Transform *ptransform = nullptr;
  auto view = registry.view<Player, Transform>();
  for (auto entity : view) {
    // This assert ensures this loop only runs once.
    assert(nullptr == ptransform);

    ptransform = &view.get<Transform>(entity);
  }
  return *ptransform;
}

auto
init(stlw::Logger &logger, entt::DefaultRegistry &registry, ImGuiIO &imgui,
     window::Dimensions const &dimensions, LoadedEntities const &entities_from_file)
{
  auto const fheight = dimensions.h;
  auto const fwidth = dimensions.w;
  auto const aspect = static_cast<GLfloat>(fwidth / fheight);

  // Initialize opengl
  render::init(dimensions);

  // Configure Imgui
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  // Construct tilemap
  stlw::float_generator rng;
  auto tmap_startingpos = level_generator::make_tilemap(80, 1, 45, rng);
  auto tmap = MOVE(tmap_startingpos.first);
  auto &startingpos = tmap_startingpos.second;

  // Construct entities
  std::vector<Transform *> entities;
  registry.view<Transform>().each(
      [&entities](auto entity, auto &transform) { entities.emplace_back(&transform); });
  assert(0 < entities.size());

  // for now assume only 1 entity has the Light tag
  assert(1 == registry.view<Light>().size());

  auto light_view = registry.view<Light, Transform>();
  for (auto const entity : light_view) {
    auto &transform = light_view.get<Transform>(entity);
    transform.scale = glm::vec3{0.2f};
    transform.translation = glm::vec3{startingpos.x, startingpos.y, startingpos.z};
  }

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -opengl::Z_UNIT_VECTOR;
  auto constexpr UP = opengl::Y_UNIT_VECTOR;

  auto &player_transform = find_player(registry);
  player_transform.rotation = glm::angleAxis(glm::radians(180.0f), opengl::Y_UNIT_VECTOR);

  WorldObject player{player_transform, FORWARD, UP};
  Projection const proj{90.0f, 4.0f / 3.0f, 0.1f, 200.0f};
  Camera camera(proj, player_transform, FORWARD, UP);
  GameState gs{logger,     imgui,          dimensions,   MOVE(rng),
               MOVE(tmap), MOVE(entities), MOVE(camera), MOVE(player)};
  return gs;
}

void
draw_entities(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
    HandleManager &handles)
{
  auto const draw_fn = [&handles, &sps, &state](auto entity, auto &sn, auto &transform) {
    auto &shader_ref = sps.ref_sp(sn.value);
    auto &handle = handles.lookup(entity);
    render::draw(state.render_args(), transform, shader_ref, handle);
  };

  auto const draw_adapter = [&](auto entity, auto &sn, auto &transform, auto &) {
    draw_fn(entity, sn, transform);
  };

  //
  // Actual drawing begins here
  registry.view<ShaderName, Transform, CubeRenderable>().each(draw_adapter);
  registry.view<ShaderName, Transform, MeshRenderable>().each(draw_adapter);

  auto const draw_skybox = [&](auto entity, auto &sn, auto &transform, auto &) {
    if (state.render.draw_skybox) {
      draw_fn(entity, sn, transform);
    }
  };
  registry.view<ShaderName, Transform, SkyboxRenderable>().each(draw_skybox);
}

void
draw_tilemap(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
    HandleManager &handles)
{
  auto &logger = state.logger;

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

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
  auto &transform = registry.assign<Transform>(entity);
  auto &hash_sp = sps.ref_sp("hashtag");
  auto &plus_sp = sps.ref_sp("plus");

  auto const load_normals = opengl::LoadNormals{true};
  auto const load_uvs = opengl::LoadUvs{false};

  auto loader = opengl::ObjLoader{LOC::WHITE};
  auto hashtag_obj = loader.load_mesh("assets/hashtag.obj", "assets/hashtag.mtl", load_normals, load_uvs);
  auto hashtag_handle = OF::copy_gpu(logger, GL_TRIANGLES, hash_sp, hashtag_obj, boost::none);

  //auto &hash_handle = handles.get(
  auto plus_obj = loader.load_mesh("assets/plus.obj", "assets/plus.mtl", load_normals, load_uvs);
  auto plus_handle = OF::copy_gpu(logger, GL_TRIANGLES, plus_sp, plus_obj, boost::none);

  render::draw_tilemap(state.render_args(), transform,
      {hashtag_handle, hash_sp, plus_handle, plus_sp},
      state.tilemap, state.render.tilemap.reveal);
}

void
draw_tilegrid(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps)
{
  auto &logger = state.logger;

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);

  auto &sp = sps.ref_sp("3d_pos_color");
  auto const tilegrid =
      OF::create_tilegrid(logger, sp, state.tilemap, state.render.tilemap.show_yaxis_lines);
  render::draw_tilegrid(state.render_args(), transform, sp, tilegrid);
}

void
draw_global_axis(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps)
{
  auto &logger = state.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto world_arrows = OF::create_world_axis_arrows(logger, sp);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);

  auto const& rargs = state.render_args();
  render::draw(rargs, transform, sp, world_arrows.x_dinfo);
  render::draw(rargs, transform, sp, world_arrows.y_dinfo);
  render::draw(rargs, transform, sp, world_arrows.z_dinfo);
}

void
draw_local_axis(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
    glm::vec3 const& player_pos)
{
  auto &logger = state.logger;
  auto &sp = sps.ref_sp("3d_pos_color");
  auto const axis_arrows = OF::create_axis_arrows(logger, sp, player_pos);

  auto entity = registry.create();
  ON_SCOPE_EXIT([&]() { registry.destroy(entity); });

  auto &transform = registry.assign<Transform>(entity);

  auto const& rargs = state.render_args();
  render::draw(rargs, transform, sp, axis_arrows.x_dinfo);
  render::draw(rargs, transform, sp, axis_arrows.y_dinfo);
  render::draw(rargs, transform, sp, axis_arrows.z_dinfo);
}

void
draw_target_vectors(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
    WorldObject const& player)
{
  auto &logger = state.logger;
  auto &sp = sps.ref_sp("3d_pos_color");

  auto const draw_arrow = [&](auto const& start, auto const& head, auto const& color) {
    auto const handle = OF::create_arrow(logger, sp, OF::ArrowCreateParams{color, start, head});

    auto entity = registry.create();
    ON_SCOPE_EXIT([&]() { registry.destroy(entity); });
    auto &transform = registry.assign<Transform>(entity);

    auto const& rargs = state.render_args();
    render::draw(rargs, transform, sp, handle);
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
    glm::vec3 const head = state.ui_state.last_mouse_clicked_pos;
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
game_loop(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
          window::SDLWindow &window, HandleManager &handles, Assets const &assets)
{
  auto &player = state.player;
  auto &mouse = state.mouse;
  auto &render = state.render;
  auto &logger = state.logger;

  // game logic
  if (mouse.right_pressed && mouse.left_pressed) {
    player.move(0.25f, player.forward_vector());
    render.tilemap.redraw = true;
  }

  // compute tilemap
  if (render.tilemap.redraw) {
    LOG_INFO("Updating tilemap\n");
    update_visible_tiles(state.tilemap, player, render.tilemap.reveal);

    // We don't need to recompute the tilemap, we just did.
    render.tilemap.redraw = false;
  }

  // action begins here
  render::clear_screen(render.background);

  if (render.draw_entities) {
    draw_entities(state, registry, sps, handles);
  }
  if (render.draw_tilemap) {
    draw_tilemap(state, registry, sps, handles);
  }

  if (render.tilemap.show_grid_lines) {
    draw_tilegrid(state, registry, sps);
  }
  if (render.show_global_axis) {
    draw_global_axis(state, registry, sps);
  }
  if (render.show_local_axis) {
    draw_local_axis(state, registry, sps, player.world_position());
  }
  if (render.show_target_vectors) {
    draw_target_vectors(state, registry, sps, player);
  }
  // UI code
  draw_ui(state, window);
}

} // ns boomhs

namespace
{

using State = boomhs::GameState;

struct Engine {
  ::window::SDLWindow window;

  MOVE_CONSTRUCTIBLE_ONLY(Engine);

  auto dimensions() const { return window.get_dimensions(); }
};

void
loop(Engine &engine, State &state, entt::DefaultRegistry &registry, boomhs::HandleManager &handles,
     opengl::ShaderPrograms &sp, Assets const &assets)
{
  auto &logger = state.logger;
  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  SDL_Event event;
  boomhs::IO::process(state, event);
  boomhs::game_loop(state, registry, sp, engine.window, handles, assets);

  // Render Imgui UI
  ImGui::Render();

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(engine.window.raw());
}

void
timed_game_loop(entt::DefaultRegistry &registry, Engine &engine, boomhs::GameState &state,
                boomhs::HandleManager &handles, opengl::ShaderPrograms &sp, Assets const &assets)
{
  auto &logger = state.logger;

  int frames_counted = 0;
  window::LTimer fps_timer;

  while (!state.quit) {
    window::LTimer frame_timer;
    auto const start = frame_timer.get_ticks();
    loop(engine, state, registry, handles, sp, assets);

    uint32_t const frame_ticks = frame_timer.get_ticks();
    float constexpr ONE_60TH_OF_A_FRAME = (1 / 60) * 1000;

    if (frame_ticks < ONE_60TH_OF_A_FRAME) {
      LOG_TRACE("Frame finished early, sleeping rest of frame.");
      SDL_Delay(ONE_60TH_OF_A_FRAME - frame_ticks);
    }

    float const fps = frames_counted / (fps_timer.get_ticks() / 1000.0f);
    LOG_INFO(fmt::format("average FPS '{}'", fps));
    ++frames_counted;
  }
}

inline stlw::result<stlw::empty_type, std::string>
start(stlw::Logger &logger, Engine &engine)
{
  using namespace opengl;

  entt::DefaultRegistry registry;

  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  LOG_TRACE("Loading assets.");
  DO_TRY(auto asset_pair, boomhs::load_assets(logger, registry));
  auto assets = MOVE(asset_pair.first);
  auto shader_programs = MOVE(asset_pair.second);

  LOG_TRACE("Copy assets to GPU.");
  DO_TRY(auto drawinfos,
         boomhs::copy_assets_gpu(logger, shader_programs, registry, assets.obj_cache));

  auto &imgui = ImGui::GetIO();
  auto state = boomhs::init(logger, registry, imgui, engine.dimensions(), assets.loaded_entities);

  timed_game_loop(registry, engine, state, drawinfos, shader_programs, assets);
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
