#include <cstdlib>
#include <memory>

#include <engine/gfx/gfx.hpp>
#include <engine/window/window.hpp>
#include <game/game.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>

#include <engine/gfx/opengl_gfx.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/boomhs/boomhs.hpp>

int
main(int argc, char *argv[])
{
  auto logger = stlw::log_factory::make_logger("main logger", "DELME.log", "txt", 23, 59);
  auto const on_error = [&](auto const &error) {
    logger.error(error);
    return EXIT_FAILURE;
  };

  // Select windowing library as SDL.
  namespace w = engine::window;
  using window_lib = w::library_wrapper<w::sdl_library>;

  logger.debug("Initializing window library globals.");
  DO_MONAD_OR_ELSE_RETURN(auto _, window_lib::init(), on_error);

  logger.debug("Setting up stack guard to unitialize window library globals.");
  ON_SCOPE_EXIT([]() { window_lib::destroy(); });

  logger.debug("Instantiating window instance.");
  DO_MONAD_OR_ELSE_RETURN(auto window, window_lib::make_window(), on_error);

  // Initialize graphics renderer
  namespace gfx = engine::gfx;
  using gfx_lib = gfx::library_wrapper<gfx::opengl::opengl_library>;
  DO_MONAD_OR_ELSE_RETURN(auto renderer, gfx_lib::make_renderer(std::move(window)), on_error);

  // Selecting the game
  using game_factory = game::game_factory;
  using game_lib = game::boomhs::boomhs_library;

  // Initialize the game instance.
  logger.debug("Instantiating boomhs instance.");
  auto game = game_factory::make_game(logger);

  logger.debug("Starting boomhs game loop.");
  DO_MONAD_OR_ELSE_RETURN(auto __, (game.run<decltype(renderer), game_lib>(std::move(renderer))),
                          on_error);

  logger.debug("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
