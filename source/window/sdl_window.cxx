#include <opengl/glew.hpp>
#include <window/sdl_window.hpp>
#include <stlw/format.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace window
{

// std::string error = SDL_GetError();
// if (error != "") {
// ostream << "SLD Error : " << error << std::endl;
// SDL_ClearError();
//}

stlw::result<stlw::empty_type, std::string>
sdl_library::init()
{
  // (from the docs) The requested attributes should be set before creating an
  // OpenGL window
  auto const set_attribute = [](auto const attribute,
                                auto const value) -> stlw::result<stlw::empty_type, std::string> {
    bool const set_attribute_suceeded = SDL_GL_SetAttribute(attribute, value);
    if (!set_attribute_suceeded) {
      auto const fmt = fmt::format("Setting attribute '{}' failed, error is '{}'\n",
                                   std::to_string(attribute), SDL_GetError());
      return stlw::make_error(fmt);
    }
    return stlw::make_empty();
  };

  // Use OpenGL 3.1 core
  DO_TRY(auto _, set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
  DO_TRY(auto __, set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 1));
  DO_TRY(auto ___, set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));

  // Turn on double buffering with a 24bit Z buffer.
  // You may need to change this to 16 or 32 for your system
  DO_TRY(auto ____, set_attribute(SDL_GL_DOUBLEBUFFER, 1));

  // Use v-sync
  SDL_GL_SetSwapInterval(1);

  // Initialize video subsystem
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    // Display error message
    auto const error = fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
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
sdl_library::make_window(int const height, int const width)
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
  // will handle it (by design).

  // First, create the SDL window.
  auto const title = "Hello World!";
  auto const flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto raw = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                              height, flags);
  if (nullptr == raw) {
    auto const error = fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }
  window_ptr window_ptr{raw, &SDL_DestroyWindow};

  // Second, create the graphics context.
  auto gl_context = SDL_GL_CreateContext(window_ptr.get());
  if (nullptr == gl_context) {
    // Display error message
    auto const error =
        fmt::format("OpenGL context could not be created! SDL Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }
  // Make the window the current one
  SDL_GL_MakeCurrent(window_ptr.get(), gl_context);

  // make sdl capture the input device
  // http://gamedev.stackexchange.com/questions/33519/trap-mouse-in-sdl
  {
    int const code = SDL_SetRelativeMouseMode(SDL_TRUE);
    if (code == -1) {
      return stlw::make_error("Mouse relative mode not supported.");
    }
    else if (code != 0) {
      return stlw::make_error(fmt::sprintf("Error setting mouse relative mode '%s'", SDL_GetError()));
    }
  }

  // set window initially to rhs
  {
    SDL_DisplayMode dmode;
    SDL_GetDesktopDisplayMode(0, &dmode);

    auto const h = dmode.h;
    auto const w = dmode.w;
    SDL_SetWindowPosition(window_ptr.get(), w / 2, h / 2);
  }

  // Third, initialize GLEW.
  glewExperimental = GL_TRUE;
  auto const glew_status = glewInit();
  if (GLEW_OK != glew_status) {
    auto const error =
        fmt::format("GLEW could not initialize! GLEW error: {}\n", glewGetErrorString(glew_status));
    return stlw::make_error(error);
  }
  return sdl_window{MOVE(window_ptr), gl_context};
}

} // ns window
