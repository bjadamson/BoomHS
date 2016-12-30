#pragma once
#include <memory>
#include <string>

#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <window/window.hpp>
#include <window/sdl.hpp>

namespace window
{

using window_type = SDL_Window;
using window_ptr = std::unique_ptr<window_type, decltype(&SDL_DestroyWindow)>;

class sdl_window
{
  window_ptr window_;
  SDL_GLContext context_;

public:
  // ctors
  sdl_window(window_ptr &&w, SDL_GLContext c)
      : window_(std::move(w))
      , context_(c)
  {
  }
  ~sdl_window()
  {
    if (nullptr != this->context_) {
      SDL_GL_DeleteContext(this->context_);
    }
  }

  NO_COPY(sdl_window)

  // move-constructible
  sdl_window(sdl_window &&other)
      : window_(std::move(other.window_))
      , context_(other.context_)
  {
    other.context_ = nullptr;
    other.window_ = nullptr;
  }

  // not move assignable
  sdl_window &operator=(sdl_window &&) = delete;

  // Allow getting the window's SDL pointer
  window_type *raw() { return this->window_.get(); }

  dimensions get_dimensions() const
  {
    int w = 0, h = 0;
    assert(nullptr != this->window_.get());
    SDL_GetWindowSize(this->window_.get(), &w, &h);
    return {w, h};
  }
};

struct sdl_library {
  sdl_library() = delete;

  static stlw::result<stlw::empty_type, std::string> init();
  static void destroy();

  static stlw::result<sdl_window, std::string> make_window(int const, int const);
};

} // ns window