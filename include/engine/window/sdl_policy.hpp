#pragma once
#include <engine/window/sdl_window.hpp>

namespace engine
{
namespace window
{

struct sdl_policy
{
  sdl_policy() = delete;
  inline static decltype(auto) init () { return sdl_window::init(); }
  inline static void destroy() { sdl_window::destroy(); }
  inline static decltype(auto) make() { return sdl_window::make(); }

  using window_type = sdl_window;
};

} // ns window
} // ns engine
