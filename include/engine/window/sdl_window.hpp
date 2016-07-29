#pragma once
#include <memory>
#include <string>

#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>
#include <extlibs/sdl.hpp>

namespace engine
{
namespace window
{

using window_type = SDL_Window;
using window_ptr = std::unique_ptr<window_type, decltype(&SDL_DestroyWindow)>;
class sdl_window
{
  window_ptr w_;
public:
  // ctors
  sdl_window(window_ptr &&w) : w_(std::move(w)) {}

  NO_COPY(sdl_window)
  MOVE_DEFAULT(sdl_window)

  static stlw::result<stlw::empty_type, std::string>
  init();

  static void
  uninit();

  static stlw::result<sdl_window, std::string>
  make();

  // Allow getting the window's SDL pointer
  window_type* raw() { return this->w_.get(); }
};

} // ns window
} // ns engine
