#pragma once
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>

#include <opengl/pipelines.hpp>
#include <opengl/factory.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <stlw/random.hpp>
#include <stlw/type_ctors.hpp>

// TODO: decouple??
#include <boomhs/ecst.hpp>
#include <boomhs/assets.hpp>
#include <boomhs/io.hpp>
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

template<typename PROXY>
void loop(Engine &engine, State &state, PROXY &proxy, game::Assets const& assets)
{
  auto &logger = state.logger;
  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  SDL_Event event;
  boomhs::IO::process(state, event);
  auto &imgui = ImGui::GetIO();
  {
    auto const exec = [&state](auto &system, auto &data) {
      system.process(data, state);
    };
    auto const exec_system = [&exec](auto &tv) {
      namespace sea = ecst::system_execution_adapter;
      auto tag = sea::t(tv);
      return tag.for_subtasks(exec);
    };
    proxy.execute_systems()(
        // ADD MORE SYSTEMS HERE
        exec_system(st::randompos_system));
  }

  game::game_loop(state, proxy, engine.opengl_lib, engine.window, assets);

  // Render Imgui UI
  ImGui::Render();

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(engine.window.raw());
}

template<typename PROXY>
void
timed_game_loop(PROXY &proxy, Engine &engine, boomhs::GameState &state, boomhs::Assets const& assets)
{
  auto &logger = state.logger;

  int frames_counted = 0;
  window::LTimer fps_timer;

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
}

inline void
start(stlw::Logger &logger, Engine &engine)
{
  using namespace opengl;

  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  // Create an ECST context.
  LOG_TRACE("creating ecst context ...");
  auto ctx = ecst_setup::make_context();

  LOG_TRACE("stepping ecst once");
  auto state = ctx->step([&logger, &engine](auto &proxy) {
      auto const dimensions = engine::get_dimensions(engine);
      auto &imgui = ImGui::GetIO();
      return game::init(logger, proxy, imgui, dimensions);
  });

  LOG_TRACE("Loading assets.");
  auto const assets = game::load_assets(state.logger, engine.opengl_lib);

  auto const init = [&state](auto &system, auto &tdata) { system.init(tdata, state); };
  auto const system_init = [&init](auto &tv) {
    namespace sea = ecst::system_execution_adapter;
    auto tag = sea::t(tv);
    return tag.for_subtasks(init);
  };

  ctx->step([&](auto &proxy) {
    LOG_TRACE("game started, initializing systems.");
    proxy.execute_systems()(
        // ADD MORE SYSTEMS HERE
        system_init(st::randompos_system));

    LOG_TRACE("systems initialized, entering main loop.");
    timed_game_loop(proxy, engine, state, assets);
    LOG_TRACE("game loop finished.");
  });
}

} // ns engine
