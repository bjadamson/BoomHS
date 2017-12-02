#pragma once
#include <opengl/lib.hpp>
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

namespace engine
{

struct Engine
{
  ::window::sdl_window window;
  opengl::OpenglPipelines gfx_lib;

  explicit Engine(::window::sdl_window &&w, opengl::OpenglPipelines &&gfxlib)
      : window(MOVE(w))
      , gfx_lib(MOVE(gfxlib))
  {
  }
  MOVE_CONSTRUCTIBLE_ONLY(Engine);
};

void begin() { opengl::begin(); }

void end(Engine &e)
{
  opengl::end();

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(e.window.raw());
}

template<typename State, typename Game, typename P, typename ASSETS>
void loop(Engine &engine, State &state, Game &game, P &proxy, ASSETS const& assets)
{
  namespace sea = ecst::system_execution_adapter;
  auto io_tags = sea::t(st::io_system);
  auto randompos_tags = sea::t(st::randompos_system);

  auto const process_system = [&state](auto &system, auto &data) {
    system.process(data, state);
  };
  auto const io_process_system = io_tags.for_subtasks(process_system);
  auto const randompos_process_system = randompos_tags.for_subtasks(process_system);

  auto &logger = state.logger;
  LOG_TRACE("executing systems.");
  proxy.execute_systems()(io_process_system, randompos_process_system);

  begin();
  LOG_TRACE("rendering.");

  game.game_loop(state, engine.gfx_lib, assets);
  end(engine);
  LOG_TRACE("game loop stepping.");
}

template <typename G, typename S>
void start(Engine &engine, G &game, S &state)
{
  using namespace opengl;

  auto &logger = state.logger;
  LOG_TRACE("creating ecst context ...");

  // Create an ECST context.
  auto ctx = ecst_setup::make_context();
  LOG_TRACE("stepping ecst once");

  auto const assets = ctx->step([&](auto &proxy) {
      return game.init(proxy, state, engine.gfx_lib);
  });

  namespace sea = ecst::system_execution_adapter;
  auto io_tags = st::io_system;
  auto randompos_tags = st::randompos_system;

  auto const init_system = [&logger](auto &system, auto &) { system.init(logger); };
  [&init_system](auto &system) { sea::t(system).for_subtasks(init_system); };

  //auto game_systems = game.ecst_systems();//MOVE(io_tags), MOVE(randompos_tags));
  //stlw::for_each(game_systems, init);

  auto const io_init_system = sea::t(io_tags).for_subtasks(init_system);
  auto const randompos_init_system = sea::t(randompos_tags).for_subtasks(init_system);

  int frames_counted = 0;
  window::LTimer fps_timer;
  auto const fps_capped_game_loop = [&](auto const& fn) {
    window::LTimer frame_timer;
    auto const start = frame_timer.get_ticks();
    fn();

    uint32_t const frame_ticks = frame_timer.get_ticks();
    float constexpr ONE_60TH_OF_A_FRAME = (1/60) * 1000;

    if (frame_ticks < ONE_60TH_OF_A_FRAME) {
      LOG_TRACE("Frame finished early, sleeping rest of frame.");
      SDL_Delay(ONE_60TH_OF_A_FRAME - frame_ticks);
    }

    float const fps = frames_counted / (fps_timer.get_ticks() / 1000.0f);
    LOG_INFO(fmt::format("average FPS '{}'", fps));
    ++frames_counted;
  };

  auto const game_loop = [&](auto &proxy) {
    auto const fn = [&]()
    {
      loop(engine, state, game, proxy, assets);
    };
    while (!state.quit) {
      fps_capped_game_loop(fn);
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

auto get_dimensions(Engine const& e) { return e.window.get_dimensions(); }

template <typename W, typename GFX_LIB>
auto
make_engine(stlw::Logger &logger, W &&window, GFX_LIB &&gfx_lib)
{
  return Engine{MOVE(window), MOVE(gfx_lib)};
}

} // ns engine
