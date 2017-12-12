#pragma once
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>

#include <opengl/pipelines.hpp>
#include <opengl/renderer.hpp>
#include <opengl/factory.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <stlw/random.hpp>
#include <stlw/type_ctors.hpp>

// TODO: decouple??
#include <boomhs/ecst.hpp>
#include <boomhs/assets.hpp>
#include <boomhs/boomhs.hpp>

namespace engine
{

namespace game = boomhs;
using State = game::GameState;

struct Engine
{
  ::window::SDLWindow window;
  opengl::OpenglPipelines opengl_lib;

  MOVE_CONSTRUCTIBLE_ONLY(Engine);
};

inline auto
get_dimensions(Engine const& e)
{
  return e.window.get_dimensions();
}

template<typename P>
void loop(Engine &engine, State &state, P &proxy, game::Assets const& assets)
{
  namespace sea = ecst::system_execution_adapter;
  auto io_tags = sea::t(st::io_system);
  auto randompos_tags = sea::t(st::randompos_system);

  auto &logger = state.logger;
  SDL_Event event;
  auto const process_system = [&state, &event](auto &system, auto &data) {
    system.process(data, state, event);
  };
  auto const io_process_system = io_tags.for_subtasks(process_system);
  auto const randompos_process_system = randompos_tags.for_subtasks(process_system);

  LOG_TRACE("executing systems.");
  proxy.execute_systems()(
      io_process_system,
      randompos_process_system);

  // Pass SDL events to GUI library.
  ImGui_ImplSdlGL3_ProcessEvent(&event);

  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  LOG_TRACE("rendering opengl.");
  game::game_loop(state, engine.opengl_lib, assets);

  // Render Imgui UI
  ImGui::Render();

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(engine.window.raw());

  LOG_TRACE("game loop stepping.");
}

inline void start(stlw::Logger &logger, Engine &engine)
{
  using namespace opengl;

  LOG_TRACE("Loading assets.");
  auto const assets = game::load_assets(logger, engine.opengl_lib);

  // Create an ECST context.
  LOG_TRACE("creating ecst context ...");
  auto ctx = ecst_setup::make_context();

  namespace sea = ecst::system_execution_adapter;
  auto io_tags = st::io_system;
  auto randompos_tags = st::randompos_system;

  auto const init_system = [&logger](auto &system, auto &) { system.init(logger); };
  [&init_system](auto &system) { sea::t(system).for_subtasks(init_system); };

  //auto game_systems = game.ecst_systems();//MOVE(io_tags), MOVE(randompos_tags));
  //stlw::for_each(game_systems, init);

  auto const io_init_system = sea::t(io_tags).for_subtasks(init_system);
  auto const randompos_init_system = sea::t(randompos_tags).for_subtasks(init_system);

  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  LOG_TRACE("stepping ecst once");
  auto state = ctx->step([&logger, &engine](auto &proxy) {
      auto const dimensions = engine::get_dimensions(engine);
      auto &imgui = ImGui::GetIO();
      return game::init(logger, proxy, imgui, dimensions);
  });

  int frames_counted = 0;
  window::LTimer fps_timer;
  auto const game_loop = [&](auto &proxy) {
    while (!state.quit) {
      window::LTimer frame_timer;
      auto const start = frame_timer.get_ticks();
      loop(engine, state, proxy, assets);

      uint32_t const frame_ticks = frame_timer.get_ticks();
      float constexpr ONE_60TH_OF_A_FRAME = (1/60) * 1000;

      if (frame_ticks < ONE_60TH_OF_A_FRAME) {
        LOG_TRACE("Frame finished early, sleeping rest of frame.");
        SDL_Delay(ONE_60TH_OF_A_FRAME - frame_ticks);
      }

      float const fps = frames_counted / (fps_timer.get_ticks() / 1000.0f);
      LOG_INFO(fmt::format("average FPS '{}'", fps));
      ++frames_counted;
      }
  };
  ctx->step([&](auto &proxy) {
    LOG_TRACE("game started, initializing systems.");
    proxy.execute_systems()(io_init_system, randompos_init_system);
    //proxy.execute_systmes()(MOVE(game_systems));
    LOG_TRACE("systems initialized, entering main loop.");

    game_loop(proxy);
    LOG_TRACE("game loop finished.");
  });
}

} // ns engine
