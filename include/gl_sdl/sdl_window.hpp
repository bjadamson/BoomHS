#pragma once
#include <boomhs/screen_info.hpp>

#include <common/auto_resource.hpp>
#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <cassert>
#include <extlibs/sdl.hpp>
#include <memory>
#include <string>
#include <type_traits>

namespace gl_sdl
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

struct WindowState
{
  MOVE_CONSTRUCTIBLE_ONLY(WindowState);

  FullscreenFlags  fullscreen = FullscreenFlags::NOT_FULLSCREEN;
  SwapIntervalFlag sync       = SwapIntervalFlag::SYNCHRONIZED;
};

using window_type = SDL_Window;
using window_ptr  = std::unique_ptr<window_type, decltype(&SDL_DestroyWindow)>;

class SDLWindow
{
  window_ptr    window_;
  SDL_GLContext context_;

  NO_COPY(SDLWindow);
  NO_MOVE_ASSIGN(SDLWindow);

public:
  SDLWindow(window_ptr&&, SDL_GLContext);
  ~SDLWindow();

  // move-constructible
  SDLWindow(SDLWindow&&);

  // methods
  boomhs::Viewport viewport() const;

  // Allow getting the window's SDL pointer
  window_type* raw() { return window_.get(); }

  void set_fullscreen(FullscreenFlags const);
  bool try_set_swapinterval(SwapIntervalFlag const);
  void set_swapinterval(SwapIntervalFlag const);
};

class SDLGlobalContext;
using SDLContext = common::AutoResource<SDLGlobalContext>;
class SDLGlobalContext
{
  SDLGlobalContext() = default;

public:
  MOVE_CONSTRUCTIBLE_ONLY(SDLGlobalContext);

  Result<SDLWindow, std::string> make_window(common::Logger&, char const*, bool, int, int) const;

  void destroy_impl();

  static Result<SDLContext, std::string> create(common::Logger&);
};

} // namespace gl_sdl
