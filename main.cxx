#include <cstdlib>
#include <string>

#include <stlw/type_macros.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <engine/lib.hpp>
#include <opengl/lib.hpp>
#include <window/sdl_window.hpp>

#include <game/boomhs/boomhs.hpp>

using EngineResult = stlw::result<engine::Engine, std::string>;

template<typename L>
EngineResult
make_opengl_sdl_premade_configuration(L &logger, float const width, float const height)
{
  // Select windowing library as SDL.
  LOG_DEBUG("Initializing window library globals");
  DO_TRY(auto _, window::sdl_library::init());

  LOG_DEBUG("Instantiating window instance.");
  DO_TRY(auto window, window::sdl_library::make_window(height, width));

  DO_TRY(auto opengl, opengl::lib_factory::make(logger));
  return engine::make_engine(logger, MOVE(window), MOVE(opengl));
}

int
main(int argc, char *argv[])
{
  auto logger = stlw::log_factory::make_default_logger("main logger");
  auto const on_error = [&](auto const &error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  DO_TRY_OR_ELSE_RETURN(auto engine,
      make_opengl_sdl_premade_configuration(logger, 800, 600),
      on_error);

  LOG_DEBUG("Instantiating 'state'");
  auto const dimensions = engine::get_dimensions(engine);
  auto state = game::boomhs::make_state(logger, dimensions);

  // Initialize the game instance.
  LOG_DEBUG("Instantiating game 'boomhs'");
  game::boomhs::boomhs_game game{engine.gfx_lib};

  LOG_DEBUG("Starting game loop");
  engine::start(engine, MOVE(game), MOVE(state));

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
