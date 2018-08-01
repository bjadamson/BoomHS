#pragma once
#include <boomhs/dimensions.hpp>

#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <cassert>
#include <extlibs/sdl.hpp>
#include <memory>
#include <string>
#include <type_traits>

namespace window
{

// clang-format off
enum class FullscreenFlags
{
  NOT_FULLSCREEN     = 0x0,
  FULLSCREEN         = 0x1,
  FULLSCREEN_DESKTOP = 0x2,
  //                 = 0x4
};

enum class SwapIntervalFlag
{
  IMMEDIATE    = 0x0,
  SYNCHRONIZED = 0x1,
  LATE_TEARING = 0x2,
  //           = 0x4
};
// clang-format on

using window_type = SDL_Window;
using window_ptr  = std::unique_ptr<window_type, decltype(&SDL_DestroyWindow)>;

class SDLWindow
{
  window_ptr    window_;
  SDL_GLContext context_;

public:
  // ctors
  SDLWindow(window_ptr&& w, SDL_GLContext c)
      : window_(MOVE(w))
      , context_(c)
  {
  }
  ~SDLWindow()
  {
    if (nullptr != context_) {
      SDL_GL_DeleteContext(context_);
    }
  }

  NO_COPY(SDLWindow)

  // move-constructible
  SDLWindow(SDLWindow&& other)
      : window_(MOVE(other.window_))
      , context_(other.context_)
  {
    other.context_ = nullptr;
    other.window_  = nullptr;
  }

  // not move assignable
  SDLWindow& operator=(SDLWindow&&) = delete;

  // Allow getting the window's SDL pointer
  window_type* raw() { return window_.get(); }

  auto get_dimensions() const
  {
    int w = 0, h = 0;
    assert(nullptr != window_.get());
    SDL_GetWindowSize(window_.get(), &w, &h);

    int x, y;
    SDL_GetWindowPosition(window_.get(), &x, &y);
    return boomhs::Dimensions{0, 0, w, h};
  }

  void set_fullscreen(FullscreenFlags const);

  bool try_set_swapinterval(SwapIntervalFlag const);

  void set_swapinterval(SwapIntervalFlag const);
};

struct sdl_library
{
  sdl_library() = delete;

  static Result<stlw::none_t, std::string> init(stlw::Logger&);
  static void                              destroy();

  static Result<SDLWindow, std::string> make_window(stlw::Logger&, bool, int, int);
};

} // namespace window
