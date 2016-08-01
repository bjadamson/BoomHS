#include <memory>
#include <cstdlib>

#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <engine/window/window.hpp>

#include <game/game.hpp>
#include <game/boomhs/boomhs.hpp>

int
main(int argc, char *argv[])
{
  auto logger = stlw::log_factory::make_logger("logfile", "txt", 23, 59);
  logger.info("Hi?");

  auto const rf = [&](auto const& error)
  {
    logger.error(error);
    return EXIT_FAILURE;
  };

  //// Here we select which "policies" we will combine to create our final "game".
  //using gamelib = game::the_library<game::boomhs::policy>;

  DO_MONAD_OR_ELSE_RETURN(auto _, engine::window::the_library::init(), rf);
  DO_MONAD_OR_ELSE_RETURN(auto window, engine::window::the_library::make_window(), rf);

  auto boomhs = game::boomhs::factory::make(std::move(window), logger);
  ON_SCOPE_EXIT( []() { engine::window::the_library::uninit(); });
  DO_MONAD_OR_ELSE_RETURN(auto __, boomhs.game_loop(), rf);
  return EXIT_SUCCESS;
}
