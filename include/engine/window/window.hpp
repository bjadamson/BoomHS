#pragma once
#include <stlw/result.hpp>
#include <engine/window/sdl_policy.hpp>

namespace engine
{
namespace window
{

// Genric template we expect a library to provide an implementation of.
template<typename P>
struct window_policy_template
{
  window_policy_template() = delete;

  static decltype(auto)
  init() { return P::init(); }

  inline static decltype(auto)
  make_window() { return P::make(); }

  inline static void
  uninit() { P::uninit(); }

  // Export this for consumption through this policy.
  using window_type = typename P::window_type;
};

// Here we choose to select our the_library sdl::policy implementation, effectively selecting SDL
// as our windowing library.
//
// Isn't that exciting kids? ...
using the_library = window_policy_template<sdl_policy>;
using window = the_library::window_type; // RE-EXPORT
} // ns window
} // engine
