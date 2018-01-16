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
        std::cerr << "copying cube with colors: '" << color << "'\n";
        auto handle = OF::copy_colorcube_gpu(logger, shader_ref, color);
        handle_list.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, opengl::Color, MeshRenderable>().each(
      [&](auto entity, auto &sn, auto &color, auto &mesh) {
        auto const &obj = obj_cache.get_obj(mesh.name);
        std::cerr << "copying mesh with colors: '" << color << "'\n";
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

auto &
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
  auto const pos = glm::vec3{startingpos.x, startingpos.y, startingpos.z};

  // Construct entities
  std::vector<Transform *> entities;
  registry.view<Transform>().each(
      [&entities](auto entity, auto &transform) { entities.emplace_back(&transform); });
  assert(0 < entities.size());

  // for now assume only 1 entity has the Light tag
  assert(1 == registry.view<Light>().size());

  auto light_view = registry.view<Light, Transform>();
  for (auto entity : light_view) {
    auto &transform = light_view.get<Transform>(entity);
    transform.scale = glm::vec3{0.2f};
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
game_loop(GameState &state, entt::DefaultRegistry &registry, opengl::ShaderPrograms &sps,
          window::SDLWindow &window, HandleManager &handles, Assets const &assets)
{
  LoadedEntities const &entities_from_file = assets.loaded_entities;
  opengl::TextureTable const &ttable = assets.texture_table;
  ObjCache const &obj_cache = assets.obj_cache;
  auto const &ents = state.entities;
  auto &player = state.player;
  auto &mouse = state.mouse;
  auto &render = state.render;
  auto &logger = state.logger;
  auto &camera = state.camera;
  auto rargs = state.render_args();

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
    state.render.tilemap.redraw = false;
  }

  // action begins here
  render::clear_screen(render.background);

  // Draw all entities which have a ShaderName component along with a Transform component
  registry.view<ShaderName, Transform>().each(
      [&handles, &sps, &rargs](auto entity, auto &sn, auto &transform) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto &handle = handles.lookup(entity);
        render::draw(rargs, transform, shader_ref, handle);
      });

  draw_tilegrid(state, registry, sps);

  /*
  // light
  {
    auto entity = registry.create();
    auto &transform = registry.assign<Transform>(entity);

    auto light_handle = OF::copy_colorcube_gpu(logger, sps.ref_sp("light"), state.light.diffuse);
    render::draw(rargs, transform, sps.ref_sp("light"), light_handle);
  }

  // skybox
  state.skybox.transform.translation = ents[AT_INDEX]->translation;
  if (state.render.draw_skybox) {
    render::draw(rargs, state.skybox.transform, sps.ref_sp("skybox"), handles.get("SKYBOX"));
  }

  // tilemap
  render::draw_tilemap(rargs, *ents[TILEMAP_INDEX],
      {handles.get("HASHTAG"), sps.ref_sp("hashtag"), handles.get("PLUS"), sps.ref_sp("plus")},
      state.tilemap, state.render.tilemap.reveal);

  // player
  render::draw(rargs, *ents[AT_INDEX], sps.ref_sp("3d_pos_normal_color"), handles.get("AT"));

  // enemies
  render::draw(rargs, *ents[ORC_INDEX], sps.ref_sp("3d_pos_normal_color"), handles.get("ORC"));
  render::draw(rargs, *ents[TROLL_INDEX], sps.ref_sp("3d_pos_normal_color"), handles.get("TROLL"));

  // global coordinates
  if (state.render.show_global_axis) {
    render::draw(rargs, *ents[GLOBAL_AXIS_X_INDEX], sps.ref_sp("3d_pos_color"),
  handles.get("GLOBAL_AXIS_X"));
    render::draw(rargs, *ents[GLOBAL_AXIS_Y_INDEX], sps.ref_sp("3d_pos_color"),
  handles.get("GLOBAL_AXIS_X"));
    render::draw(rargs, *ents[GLOBAL_AXIS_Z_INDEX], sps.ref_sp("3d_pos_color"),
  handles.get("GLOBAL_AXIS_X"));
  }

  // local coordinates
  if (state.render.show_local_axis) {
    auto const& player_pos = ents[AT_INDEX]->translation;
    {
      auto const world_coords = OF::create_axis_arrows(logger,
          sps.ref_sp("3d_pos_color"),
          sps.ref_sp("3d_pos_color"),
          sps.ref_sp("3d_pos_color"),
          player_pos);
      render::draw(rargs, *ents[LOCAL_AXIS_X_INDEX], sps.ref_sp("3d_pos_color"),
  handles.get("LOCAL_AXIS_X"));
      render::draw(rargs, *ents[LOCAL_AXIS_Y_INDEX], sps.ref_sp("3d_pos_color"),
  handles.get("LOCAL_AXIS_Y"));
      render::draw(rargs, *ents[LOCAL_AXIS_Z_INDEX], sps.ref_sp("3d_pos_color"),
  handles.get("LOCAL_AXIS_Z"));
    }
  }

  // draw forward arrow (for player)
  if (state.render.show_target_vectors) {
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + (2.0f * player.forward_vector());

      auto const handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
          OF::ArrowCreateParams{LOC::LIGHT_BLUE, start, head});

      render::draw(rargs, *ents[LOCAL_FORWARD_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
    // draw forward arrow (for camera)
    {
      //glm::vec3 const start = glm::vec3{0.0f, 0.0f, 1.0f};
      //glm::vec3 const head = glm::vec3{0.0f, 0.0f, 2.0f};
      glm::vec3 const start = glm::vec3{0, 0, 0};
      glm::vec3 const head = state.ui_state.last_mouse_clicked_pos;

      auto handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
        OF::ArrowCreateParams{LOC::YELLOW, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS0_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
    // draw forward arrow (for camera)
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.backward_vector();

      auto const handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
        OF::ArrowCreateParams{LOC::PINK, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS2_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
    // draw arrow from origin -> camera
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.right_vector();

      auto const handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
        OF::ArrowCreateParams{LOC::PURPLE, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS2_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
  }

  // terrain
  //render::draw(rargs, *ents[TERRAIN_INDEX], sps.ref_sp("terrain"), handles.terrain);

*/
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
