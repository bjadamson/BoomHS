#include <engine/gfx/opengl_glew.hpp>
#include <engine/window/sdl_window.hpp>
#include <stlw/format.hpp>
#include <stlw/type_ctors.hpp>

namespace engine
{
namespace window
{

stlw::result<stlw::empty_type, std::string>
sdl_library::init()
{
  // (from the docs) The requested attributes should be set before creating an
  // OpenGL window
  auto const set_attribute = [](auto const attribute, auto const value) {
    bool const set_attribute_suceeded = SDL_GL_SetAttribute(attribute, value);
    if (!set_attribute_suceeded) {
      std::cerr << "Setting attribute '" << std::to_string(attribute) << "' failed, error is '"
                << SDL_GetError() << "'\n";
    }
  };

  // Use OpenGL 3.1 core
  set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  // Turn on double buffering with a 24bit Z buffer.
  // You may need to change this to 16 or 32 for your system
  set_attribute(SDL_GL_DOUBLEBUFFER, 1);

  // Use v-sync
  SDL_GL_SetSwapInterval(1);

  // Initialize video subsystem
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    // Display error message
    auto const error = stlw::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }
  return stlw::make_empty();
}

void
sdl_library::destroy()
{
  if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
    SDL_Quit();
  }
}

stlw::result<sdl_window, std::string>
sdl_library::make_window()
{
  // Hidden dependency between the ordering here, so all the logic exists in one
  // place.
  //
  // * The OpenGL context MUST be initialized before the call to glewInit()
  // takes place.
  // This is because there is a hidden dependency on glew, it expects an OpenGL
  // context to be
  // initialized. The glew library knows how to find the OpenGL context in
  // memory without any
  // reference, so it's a bit like magic.
  //
  // NOTE: We don't have to do anything to shutdown glew, the processing closing
  // will handle it (by
  // design.

  // First, create the SDL window.
  auto const title = "Hello World!";
  auto const flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto const height = 800, width = 600;
  auto raw = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                              height, flags);
  if (nullptr == raw) {
    auto const error = stlw::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }
  window_ptr window_ptr{raw, &SDL_DestroyWindow};

  // Second, create the graphics context.
  auto gl_context = SDL_GL_CreateContext(window_ptr.get());
  if (nullptr == gl_context) {
    // Display error message
    auto const error =
        stlw::format("OpenGL context could not be created! SDL Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }
  SDL_GL_MakeCurrent(window_ptr.get(), gl_context);

  // Third, initialize GLEW.
  glewExperimental = GL_TRUE;
  auto const glew_status = glewInit();
  if (GLEW_OK != glew_status) {
    auto const error = stlw::format("GLEW could not initialize! GLEW error: {}\n",
        glewGetErrorString(glew_status));
    return stlw::make_error(error);
  }
  return sdl_window{std::move(window_ptr), gl_context};
}

} // ns window
} // ns engine
