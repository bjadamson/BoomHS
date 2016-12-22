#include <cstdlib>

#include <gfx/gfx.hpp>
#include <window/window.hpp>
#include <stlw/log.hpp>

#include <engine/lib.hpp>
#include <window/sdl_window.hpp>
#include <game/boomhs/boomhs.hpp>

int
main(int argc, char *argv[])
{
  auto logger = stlw::log_factory::make_default_logger("main logger");
  using L = decltype(logger);

  auto const on_error = [&](auto const &error) {
    logger.error(error);
    return EXIT_FAILURE;
  };

  // Select windowing library as SDL.
  namespace w = window;
  using window_lib = w::library_wrapper<w::sdl_library>;

  logger.debug("Initializing window library globals");
  DO_TRY_OR_ELSE_RETURN(auto _, window_lib::init(), on_error);

  logger.debug("Setting up stack guard to unitialize window library globals");
  ON_SCOPE_EXIT([]() { window_lib::destroy(); });

  auto constexpr height = 800, width = 600;
  logger.debug("Instantiating window instance.");
  DO_TRY_OR_ELSE_RETURN(auto window, window_lib::make_window(height, width), on_error);

  // Initialize graphics renderer
  auto engine = engine::factory::make_gfx_sdl_engine(logger, std::move(window));
  DO_TRY_OR_ELSE_RETURN(auto renderer, std::move(engine), on_error);
  using R = decltype(renderer);

  logger.debug("Instantiating 'state'");
  auto const dimensions = window.get_dimensions();
  auto state = game::boomhs::make_state(logger, renderer, dimensions);

  // Initialize the game instance.
  logger.debug("Instantiating game 'boomhs'");
  game::boomhs::boomhs_game game;

  logger.debug("Starting game loop");
  ecst_main(game, state);

  logger.debug("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
