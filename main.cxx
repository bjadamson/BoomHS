#include <memory>
#include <cstdlib>

#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <engine/window/window.hpp>
#include <game/game.hpp>

int
main(int argc, char *argv[])
{
  auto logger = stlw::log_factory::make_logger("logfile", "txt", 23, 59);
  logger.info("Hi?");

  auto on_error = [](auto error) {
    // TODO: use logger intead.
    std::cerr << "ERROR: '" << error << "'\n";
    return stlw::make_error(error);
  };

  auto globals_init = engine::window::the_library::init()
    .catch_error(on_error);
  if (! globals_init) {
    // If we failed to initialize the globals, just exit the program.
    return EXIT_FAILURE;
  }

  // Logic begins here, we create a window and pass it to the game_loop function
  engine::window::the_library::make_window()
    .map(game::the_library::game_loop)

    // If an error occurs during the game_loop function, on_error() is invoked exactly once before
    // control contintues past this point.
    .catch_error(on_error)
    // Make sure we shutdown the window library too!
    .map([](auto &&) { engine::window::the_library::uninit(); });
  return 0;
}
