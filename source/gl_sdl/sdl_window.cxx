#include <gl_sdl/sdl_window.hpp>
#include <gl_sdl/gl_sdl_log.hpp>

#include <common/algorithm.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>

#include <iostream>

using namespace boomhs;
using namespace gl_sdl;

namespace {

void
check_errors(common::Logger &logger)
{
  ErrorLog::abort_if_any_errors(logger);
}

Result<common::none_t, std::string>
set_attribute(common::Logger& logger, SDL_GLattr const attr, int const value)
{
  int const set_r = SDL_GL_SetAttribute(attr, value);
  check_errors(logger);
  if (0 != set_r) {
    auto const fmt = fmt::format("Setting attribute '{}' failed, error is '{}'\n",
                                  std::to_string(attr), SDL_GetError());
    std::abort();
    return Err(fmt);
  }
  return OK_NONE;
}

} // namespace anonymous

namespace gl_sdl
{

SDLWindow::SDLWindow(window_ptr&& w, SDL_GLContext c)
    : window_(MOVE(w))
    , context_(c)
{
}

SDLWindow::SDLWindow(SDLWindow&& other)
    : window_(MOVE(other.window_))
    , context_(other.context_)
{
  other.context_ = nullptr;
  other.window_  = nullptr;
}

SDLWindow::~SDLWindow()
{
  if (nullptr != context_) {
    SDL_GL_DeleteContext(context_);
  }
}

Viewport
SDLWindow::get_dimensions() const
{
  int w = 0, h = 0;
  assert(nullptr != window_.get());
  SDL_GetWindowSize(window_.get(), &w, &h);

  int x, y;
  SDL_GetWindowPosition(window_.get(), &x, &y);
  return boomhs::Viewport{0, 0, w, h};
}

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
SDLWindow::set_swapinterval(SwapIntervalFlag const swap)
{
  if (false == try_set_swapinterval(swap)) {
    std::abort();
  }
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
  if (0 != result) {
    // TODO: handle
    std::exit(1);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// SDLGlobalContext
void
SDLGlobalContext::destroy_impl()
{
  if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
    SDL_Quit();
  }
}

Result<SDLContext, std::string>
SDLGlobalContext::create(common::Logger& logger)
{
  // Initialize video subsystem
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    // Display error message
    auto const error = fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
    return Err(error);
  }
  check_errors(logger);

  return Ok(SDLContext{SDLGlobalContext{}});
}


Result<SDLWindow, std::string>
SDLGlobalContext::make_window(common::Logger &logger, char const* title, bool const fullscreen,
                              int const width, int const height) const
{
  auto const set_a = [&logger](auto const attr, auto const v) {
    return set_attribute(logger, attr, v);
  };

  // (from the docs) The requested attributes should be set before creating an
  // OpenGL window
  // Use OpenGL 3.1 core
  DO_EFFECT(set_a(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
  DO_EFFECT(set_a(SDL_GL_CONTEXT_MINOR_VERSION, 1));

  DO_EFFECT(set_a(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG));
  DO_EFFECT(set_a(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));

  // Turn on double buffering with a 24bit Z buffer.
  // You may need to change this to 16 or 32 for your system
  DO_EFFECT(set_a(SDL_GL_DOUBLEBUFFER, 1));

  DO_EFFECT(set_a(SDL_GL_DEPTH_SIZE, 24));
  DO_EFFECT(set_a(SDL_GL_STENCIL_SIZE, 8));


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
  auto const fullscreen_flag = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
  auto const flags = SDL_WINDOW_OPENGL
    | SDL_WINDOW_SHOWN
    | SDL_WINDOW_MOUSE_FOCUS
    | fullscreen_flag;
  int const x = SDL_WINDOWPOS_CENTERED;
  int const y = SDL_WINDOWPOS_CENTERED;
  auto raw = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                              height, flags);
  check_errors(logger);
  if (nullptr == raw) {
    auto const error = fmt::format("SDL could not initialize! SDL_Error: {}\n", SDL_GetError());
    return Err(error);
  }
  window_ptr window_ptr{raw, &SDL_DestroyWindow};

  // Second, create the graphics context.
  auto gl_context = SDL_GL_CreateContext(window_ptr.get());
  check_errors(logger);
  if (nullptr == gl_context) {
    // Display error message
    auto const error =
        fmt::format("OpenGL context could not be created! SDL Error: {}\n", SDL_GetError());
    return Err(error);
  }

  // Make the window the current one
  auto const mc_r = SDL_GL_MakeCurrent(window_ptr.get(), gl_context);
  check_errors(logger);
  if (0 != mc_r) {
    auto const fmt = fmt::format("Error making window current. SDL Error: {}\n", SDL_GetError());
    return Err(fmt);
  }

  // make sdl capture the input device
  // http://gamedev.stackexchange.com/questions/33519/trap-mouse-in-sdl
  {
    int const code = SDL_SetRelativeMouseMode(SDL_TRUE);
    check_errors(logger);
    if (code == -1) {
      return Err(std::string{"Mouse relative mode not supported."});
    }
    else if (code != 0) {
      return Err(fmt::sprintf("Error setting mouse relative mode '%s'", SDL_GetError()));
    }
  }

  // set window initially to rhs
  {
    SDL_DisplayMode dmode;
    SDL_GetDesktopDisplayMode(0, &dmode);
    check_errors(logger);

    auto const h = dmode.h;
    auto const w = dmode.w;
    SDL_SetWindowPosition(window_ptr.get(), 0, 0);
    check_errors(logger);
  }

  // Initially relative mode is not active. (expected user experience in most cases)
  SDL_SetRelativeMouseMode(SDL_FALSE);

  // Third, initialize GLEW.
  glewExperimental = GL_TRUE;
  check_errors(logger);
  auto const glew_status = glewInit();
  if (GLEW_OK != glew_status) {
    auto const error =
        fmt::format("GLEW could not initialize! GLEW error: {}\n", glewGetErrorString(glew_status));
    return Err(error);
  }

  // Use v-sync
  // NOTE: must happen AFTER SDL_GL_MakeCurrent call occurs.
  SDLWindow window{MOVE(window_ptr), gl_context};
  bool const success = window.try_set_swapinterval(SwapIntervalFlag::LATE_TEARING);
  if (!success) {
    // SDL fills up the log with info about the failed attempt above.
    ErrorLog::clear();

    bool const backup_success = window.try_set_swapinterval(SwapIntervalFlag::SYNCHRONIZED);
    if (!backup_success) {
      std::abort();
    }
  }
  check_errors(logger);
  return OK_MOVE(window);
}

} // ns gl_sdl
