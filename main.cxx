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

  auto const rf = [](auto &&error)
  {
    // TODO: use logger intead.
    std::cerr << "ERROR: '" << error << "'\n";
    return EXIT_FAILURE;
  };

  auto on_error = [](auto error) {
    std::cerr << "ERROR: '" << error << "'\n";
    return stlw::make_error(error);
  };

  DO_EFFECT_OR_ELSE(engine::window::the_library::init(), rf);

  // Here we select which "policies" we will combine to create our final "game".
  using gamelib = game::the_library<game::boomhs::policy>;

  //auto e_window =  engine::window::the_library::make_window();
  //if (! e_window) {
    //return EXIT_FAILURE;
  //}

  auto c = [](auto &&error) { return EXIT_FAILURE; };
  DO_MONAD_OR_ELSE(auto window, engine::window::the_library::make_window(), c);

  //auto window = std::move(*e_window);
  auto boomhs = game::boomhs::factory::make(std::move(window));

  boomhs.game_loop()
    // If an error occurs during the game_loop function, on_error() is invoked exactly once before
    // control contintues past this point.
    .catch_error(on_error)
    // Make sure we shutdown the window library too!
    .map([](auto &&) { engine::window::the_library::uninit(); });
  return 0;
}
