#pragma once
#include <engine/window/sdl_window.hpp>

namespace engine
{
namespace window
{

struct sdl_policy
{
  sdl_policy() = delete;

  using window_type = sdl_window;

  inline static decltype(auto) init () { return window_type::init(); }
  inline static void uninit() { window_type::uninit(); }

  inline static decltype(auto) make() { return window_type::make(); }

};

} // ns window
} // ns engine
