#include <window/sdl_window.hpp>
#include <stlw/format.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>
#include <gfx/gl_sdl_log.hpp>
#include <opengl/glew.hpp>

#include <iostream>

namespace {
void
check_errors()
{
  gfx::ErrorLog::abort_if_any_errors(std::cerr);
}

} // namespace anonymous

namespace window
{

bool
SDLWindow::try_set_swapinterval(SwapIntervalFlag const swap_flag)
{
  if (swap_flag == SwapIntervalFlag::IMMEDIATE) {
    // TODO: implement delay using SDL_Wait() (or someother sleep functionality)
    std::exit(1);
  }
  auto const to_int = [&](auto const flag) {
    // https://wiki.libsdl.org/SDL_GL_SetSwapInterval
    if (SwapIntervalFlag::IMMEDIATE == flag) {
      return 0;
    }
    if (SwapIntervalFlag::SYNCHRONIZED == flag) {
      return 1;
    }
    if (SwapIntervalFlag::LATE_TEARING == flag) {
      return -1;
    }

    // invalid
    std::exit(1);
    return 0;
  };
  int const r = SDL_GL_SetSwapInterval(to_int(swap_flag));
  return 0 == r;
}

void
SDLWindow::set_fullscreen(FullscreenFlags const fs)
{
  uint32_t sdl_flags = 0;
  if (fs == FullscreenFlags::NOT_FULLSCREEN) {
    // do nothing
  }
  else if (fs == FullscreenFlags::FULLSCREEN) {
    sdl_flags = SDL_WINDOW_FULLSCREEN;
  }
  else if (fs == FullscreenFlags::FULLSCREEN_DESKTOP) {
    sdl_flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
  }

  int const result = SDL_SetWindowFullscreen(window_.get(), sdl_flags);
  assert(0 == result);
}

stlw::result<stlw::empty_type, std::string>
sdl_library::init()
{
  // Initialize video subsystem
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    // Display error message
    auto const error = fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }
check_errors();

  // (from the docs) The requested attributes should be set before creating an
  // OpenGL window
  auto const set_attribute = [](auto const attribute,
                                auto const value) -> stlw::result<stlw::empty_type, std::string> {
    int const set_r = SDL_GL_SetAttribute(attribute, value);
check_errors();
    if (0 != set_r) {
      auto const fmt = fmt::format("Setting attribute '{}' failed, error is '{}'\n",
                                   std::to_string(attribute), SDL_GetError());
      std::abort();
      return stlw::make_error(fmt);
    }
    return stlw::make_empty();
  };

  // Use OpenGL 3.1 core
  DO_TRY(auto _, set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
  DO_TRY(auto __, set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 1));

  DO_TRY(auto ___, set_attribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG));
  DO_TRY(auto ____, set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));

  // Turn on double buffering with a 24bit Z buffer.
  // You may need to change this to 16 or 32 for your system
  DO_TRY(auto _____, set_attribute(SDL_GL_DOUBLEBUFFER, 1));

  DO_TRY(auto ______, set_attribute(SDL_GL_DEPTH_SIZE, 24));
  DO_TRY(auto _______, set_attribute(SDL_GL_STENCIL_SIZE, 8));

  return stlw::make_empty();
}

void
sdl_library::destroy()
{
  if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
    SDL_Quit();
  }
}

stlw::result<SDLWindow, std::string>
sdl_library::make_window(bool const fullscreen, int const height, int const width)
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
  auto const fullscreen_flag = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
  auto const flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | fullscreen_flag;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto raw = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                              height, flags);
check_errors();
  if (nullptr == raw) {
    auto const error = fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }
  window_ptr window_ptr{raw, &SDL_DestroyWindow};

  // Second, create the graphics context.
  auto gl_context = SDL_GL_CreateContext(window_ptr.get());
check_errors();
  if (nullptr == gl_context) {
    // Display error message
    auto const error =
        fmt::format("OpenGL context could not be created! SDL Error: {}\n", SDL_GetError());
    return stlw::make_error(error);
  }

  // Make the window the current one
  auto const mc_r = SDL_GL_MakeCurrent(window_ptr.get(), gl_context);
check_errors();
  if (0 != mc_r) {
    auto const fmt = fmt::format("Error making window current. SDL Error: {}\n", SDL_GetError());
    return stlw::make_error(fmt);
  }

  // make sdl capture the input device
  // http://gamedev.stackexchange.com/questions/33519/trap-mouse-in-sdl
  {
    int const code = SDL_SetRelativeMouseMode(SDL_TRUE);
check_errors();
    if (code == -1) {
      return stlw::make_error(std::string{"Mouse relative mode not supported."});
    }
    else if (code != 0) {
      return stlw::make_error(fmt::sprintf("Error setting mouse relative mode '%s'", SDL_GetError()));
    }
  }

  // set window initially to rhs
  {
    SDL_DisplayMode dmode;
    SDL_GetDesktopDisplayMode(0, &dmode);
check_errors();

    auto const h = dmode.h;
    auto const w = dmode.w;
    SDL_SetWindowPosition(window_ptr.get(), 0, 0);
check_errors();
  }

  // Third, initialize GLEW.
  glewExperimental = GL_TRUE;
check_errors();
  auto const glew_status = glewInit();
  if (GLEW_OK != glew_status) {
    auto const error =
        fmt::format("GLEW could not initialize! GLEW error: {}\n", glewGetErrorString(glew_status));
    return stlw::make_error(error);
  }

  // Use v-sync
  // NOTE: must happen AFTER SDL_GL_MakeCurrent call occurs.
  SDLWindow window{MOVE(window_ptr), gl_context};
  bool const success = window.try_set_swapinterval(SwapIntervalFlag::LATE_TEARING);
  if (!success) {
    // SDL fills up the log with info about the failed attempt above.
    gfx::ErrorLog::clear();

    bool const backup_success = window.try_set_swapinterval(SwapIntervalFlag::SYNCHRONIZED);
    if (!backup_success) {
      std::abort();
    }
  }
  check_errors();
  return window;
}

} // ns window
