#include <memory>
#include <cstdlib>

// Boost stuff
#include <boost/expected/expected.hpp>
#include <game/Sdl.hpp>

int
main(int argc, char *argv[])
{
  auto on_error = [](auto error) {
    std::cerr << "ERROR: '" << error << "'\n";
    return boost::make_unexpected(error);
  };
  auto globals_init = game::sdl::init().catch_error(on_error);
  if (! globals_init) {
    // If we failed to initialize the globals, just exit the program.
    return EXIT_FAILURE;
  }

  // Logic begins here, we create a window and pass it to the game_loop function
  game::sdl::make_window()
    .map(game::sdl::game_loop)
    // If an error occurs during the game_loop function, on_error() is invoked exactly once before
    // control contintues past this point.
    .catch_error(on_error)
    // Make sure we close down SDL too!
    .map([]() { game::sdl::destroy(); });
  return 0;
}
