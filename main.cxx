#include <cstdlib>

#include <stlw/type_macros.hpp>
#include <stlw/log.hpp>

#include <engine/lib.hpp>
#include <engine/premade.hpp>
#include <game/boomhs/boomhs.hpp>

int
main(int argc, char *argv[])
{
  auto logger = stlw::log_factory::make_default_logger("main logger");
  auto const on_error = [&](auto const &error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  auto premade_result = engine::make_opengl_sdl_premade_configuration(logger, 800, 600);
  DO_TRY_OR_ELSE_RETURN(auto premade, MOVE(premade_result), on_error);

  LOG_DEBUG("Instantiating 'state'");

  auto &engine = premade.engine();
  auto const dimensions = engine.get_dimensions();
  auto state = game::boomhs::make_state(logger, dimensions);

  // Initialize the game instance.
  LOG_DEBUG("Instantiating game 'boomhs'");
  game::boomhs::boomhs_game game;

  LOG_DEBUG("Starting game loop");
  engine.start(MOVE(game), MOVE(state));

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
