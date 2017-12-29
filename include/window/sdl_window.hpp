#pragma once
#include <memory>
#include <string>

#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>
#include <window/sdl.hpp>

namespace window
{

enum FullscreenFlags {
  NOT_FULLSCREEN = 0,
  FULLSCREEN,
  FULLSCREEN_DESKTOP
};

using window_type = SDL_Window;
using window_ptr = std::unique_ptr<window_type, decltype(&SDL_DestroyWindow)>;

class SDLWindow
{
  window_ptr window_;
  SDL_GLContext context_;

public:
  // ctors
  SDLWindow(window_ptr &&w, SDL_GLContext c)
      : window_(MOVE(w))
      , context_(c)
  {
  }
  ~SDLWindow()
  {
    if (nullptr != this->context_) {
      SDL_GL_DeleteContext(this->context_);
    }
  }

  NO_COPY(SDLWindow)

  // move-constructible
  SDLWindow(SDLWindow &&other)
      : window_(MOVE(other.window_))
      , context_(other.context_)
  {
    other.context_ = nullptr;
    other.window_ = nullptr;
  }

  // not move assignable
  SDLWindow &operator=(SDLWindow &&) = delete;

  // Allow getting the window's SDL pointer
  window_type*
  raw()
  {
    return this->window_.get();
  }

  Dimensions
  get_dimensions() const
  {
    int w = 0, h = 0;
    assert(nullptr != this->window_.get());
    SDL_GetWindowSize(this->window_.get(), &w, &h);
    return {w, h};
  }

  void
  set_fullscreen(FullscreenFlags const fs)
  {
    uint32_t sdl_flags = 0;
    if (fs == FULLSCREEN) {
      sdl_flags = SDL_WINDOW_FULLSCREEN;
    } else if (fs == FULLSCREEN_DESKTOP) {
      sdl_flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    int const result = SDL_SetWindowFullscreen(this->window_.get(), sdl_flags);
    assert(0 == result);
  }
};

struct sdl_library {
  sdl_library() = delete;

  static stlw::result<stlw::empty_type, std::string> init();
  static void destroy();

  static stlw::result<SDLWindow, std::string> make_window(bool const, int const, int const);
};

} // ns window
