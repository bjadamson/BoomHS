#include <memory>
#include <cstdlib>

// OpenGL headers
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>

// SDL headers
#include <SDL_main.h>
#include <SDL.h>
#include <SDL_opengl.h>

// Boost stuff
#include <boost/expected/expected.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>

using WindowType = SDL_Window;
using WindowPtr = std::unique_ptr<WindowType, decltype(&SDL_DestroyWindow)>;
class Window
{
  WindowPtr w_;
public:
  // ctors
  Window(WindowPtr &&w) : w_(std::move(w)) {}

  // movable, not copyable
  Window(Window &&) = default;
  Window& operator=(Window &&) = default;

  Window(Window const&) = delete;
  Window& operator=(Window const&) = delete;

  // Allow getting the window's SDL pointer
  WindowType* raw() { return this->w_.get(); }
};

boost::expected<Window, std::string>
make_window()
{
  auto const title = "Hello World!";
  auto const flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto const height = 800, width = 600;
  auto raw = SDL_CreateWindow(title, x, y, width, height, flags);
  if (nullptr == raw) {
    auto const fmt = boost::format("SDL could not initialize! SDL_Error: %s\n") % SDL_GetError();
    return boost::make_unexpected(boost::str(fmt));
  }
  WindowPtr window_ptr{raw, &SDL_DestroyWindow};
  return Window{std::move(window_ptr)};
}

struct GfxContext
{
  Window &&window;
  SDL_GLContext context; // TODO: delete on destruction

  GfxContext(Window &&w, SDL_GLContext c) : window(std::move(w)), context(c) {}

  // movable, not copyable
  GfxContext(GfxContext &&) = default;
  GfxContext& operator=(GfxContext &&) = default;

  GfxContext(GfxContext const&) = delete;
  GfxContext& operator=(GfxContext const&) = delete;
};

#define FORMAT_STRERR(ARG1) \
  boost::make_unexpected(boost::str(ARG1))

boost::expected<GfxContext, std::string>
make_gfx_context(Window &&w)
{
  auto *gl_context = SDL_GL_CreateContext(w.raw());

  if(nullptr == gl_context) {
    // Display error message
    auto const fmt = boost::format("OpenGL context could not be created! SDL Error: %s\n")
      % SDL_GetError();
    return FORMAT_STRERR(fmt);
  }

  // Hidden dependency between the ordering here, so all the logic exists in one place.
  // 
  // 1. The OpenGL context MUST be initialized before the call to glewInit() takes place.
  //    This is because there is a hidden dependency on glew, it expects an OpenGL context to be
  //    initialized. The glew library knows how to find the OpenGL context in memory without any
  //    reference, so it's a bit like magic.
  //
  // We don't have to do anything to shutdown glew, the processing closing will handle it (by
  // design).
  auto const glew_status = glewInit();
  if (GLEW_OK != glew_status) {
    auto const fmt = boost::format("GLEW could not initialize! GLEW error: %s\n")
      % glewGetErrorString(glew_status);
    return FORMAT_STRERR(fmt);
  }
  return GfxContext{std::move(w), gl_context};
}

boost::optional<std::string>
init_globals()
{
  //Use OpenGL 3.1 core
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Initialize video subsystem
  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    // Display error message
    auto const fmt = boost::format("SDL could not initialize! SDL_Error: %s\n") % SDL_GetError();
    return boost::make_optional(boost::str(fmt));
  }
  return boost::none;
}

void
main_loop(GfxContext &&w)
{
  bool quit = false;
  SDL_Event sdlEvent;

  while (!quit)
  {
    while(SDL_PollEvent(&sdlEvent) != 0)
    {
      // Esc button is pressed
      if(sdlEvent.type == SDL_QUIT) {
        quit = true;
      }
    }

    // Set background color as cornflower blue
    glClearColor(0.39f, 0.58f, 0.93f, 1.f);
    // Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(w.window.raw());
  }
}

int
main(int argc, char *argv[])
{
  auto on_error = [](auto error) {
    std::cerr << "ERROR: '" << error << "'\n";
    return boost::make_unexpected(error);
  };
  auto const init_error = init_globals();
  if ( init_error) {
    on_error(*init_error);
    return EXIT_FAILURE;
  }

  make_window()
    .bind(make_gfx_context)
    .map(main_loop)
    .catch_error(on_error);

  SDL_Quit();
  return 0;
}
