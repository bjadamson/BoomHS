#include <cstdlib>
#include <string>

#include <stlw/type_macros.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <engine/lib.hpp>
#include <opengl/pipelines.hpp>
#include <window/sdl_window.hpp>
#include <boomhs/boomhs.hpp>

using EngineResult = stlw::result<engine::Engine, std::string>;
using stlw::Logger;

EngineResult
make_opengl_sdl_engine(Logger &logger, bool const fullscreen, float const width, float const height)
{
  // Select windowing library as SDL.
  LOG_DEBUG("Initializing window library globals");
  DO_TRY(auto _, window::sdl_library::init());

  LOG_DEBUG("Instantiating window instance.");
  DO_TRY(auto window, window::sdl_library::make_window(fullscreen, height, width));

  DO_TRY(auto opengl, opengl::load_pipelines(logger));
  return engine::Engine{MOVE(window), MOVE(opengl)};
}

int
main(int argc, char *argv[])
{
  Logger logger = stlw::log_factory::make_default_logger("main logger");
  auto const on_error = [&](auto const &error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  bool constexpr FULLSCREEN = false;
  DO_TRY_OR_ELSE_RETURN(auto engine,
      make_opengl_sdl_engine(logger, FULLSCREEN, 1024, 768),
      on_error);

  LOG_DEBUG("Starting game loop");
  engine::start(logger, engine);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
