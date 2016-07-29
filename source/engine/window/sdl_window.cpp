#include <engine/window/sdl_window.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/format.hpp>

// TODO: ditch
#include <game/debug.hpp>
#include <engine/gfx/glew_gfx.hpp>
#include <engine/window/sdl_window.hpp>
#include <engine/gfx/shader.hpp>

// TODO: remove this as global
SDL_GLContext gl_context;

namespace engine
{
namespace window
{

stlw::result<stlw::empty_type, std::string>
sdl_window::init()
{
  // (from the docs) The requested attributes should be set before creating an OpenGL window
  auto const set_attribute = [](auto const attribute, auto const value)
  {
    bool const set_attribute_suceeded = SDL_GL_SetAttribute(attribute, value);
    if (! set_attribute_suceeded) {
      std::cerr << "Setting attribute '" << std::to_string(attribute) << "' failed, error is '"
        << SDL_GetError() << "'\n";
    }
  };

  //Use OpenGL 3.1 core
  set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Turn on double buffering with a 24bit Z buffer.
  // You may need to change this to 16 or 32 for your system
  set_attribute(SDL_GL_DOUBLEBUFFER, 1);

  // Use v-sync
  SDL_GL_SetSwapInterval(1);

  // TODO: move to GL / gfx module.
  // testing, remove??
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // Initialize video subsystem
  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    // Display error message
    auto const fmt = stlw::format("SDL could not initialize! SDL_Error: %s\n") % SDL_GetError();
    return FORMAT_STRERR(fmt);
  }
  return stlw::make_empty();
}

void
sdl_window::destroy()
{
  if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
    SDL_Quit();
  }
}

stlw::result<sdl_window, std::string>
sdl_window::make()
{
  // Hidden dependency between the ordering here, so all the logic exists in one place.
  //
  // * The OpenGL context MUST be initialized before the call to glewInit() takes place.
  // This is because there is a hidden dependency on glew, it expects an OpenGL context to be
  // initialized. The glew library knows how to find the OpenGL context in memory without any
  // reference, so it's a bit like magic.
  //
  // NOTE: We don't have to do anything to shutdown glew, the processing closing will handle it (by
  // design.

  // First, create the SDL window.
  auto const title = "Hello World!";
  auto const flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto const height = 800, width = 600;
  auto raw = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
  if (nullptr == raw) {
    auto const fmt = stlw::format("SDL could not initialize! SDL_Error: %s\n") % SDL_GetError();
    return stlw::make_error(stlw::to_string(fmt));
  }
  window_ptr window_ptr{raw, &SDL_DestroyWindow};

  // Second, create the graphics context.
  gl_context = SDL_GL_CreateContext(window_ptr.get());
  if(nullptr == gl_context) {
    // Display error message
    auto const fmt = stlw::format("OpenGL context could not be created! SDL Error: %s\n")
      % SDL_GetError();
    return FORMAT_STRERR(fmt);
  }
  SDL_GL_MakeCurrent(window_ptr.get(), gl_context);

  // Third, initialize GLEW.
  glewExperimental = GL_TRUE;
  auto const glew_status = glewInit();
  if (GLEW_OK != glew_status) {
    auto const fmt = stlw::format("GLEW could not initialize! GLEW error: %s\n")
      % glewGetErrorString(glew_status);
    return FORMAT_STRERR(fmt);
  }
  return sdl_window{std::move(window_ptr)};
}

} // ns window
} // ns engine
