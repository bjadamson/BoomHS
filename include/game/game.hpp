#pragma once
#include <game/boomhs_policy.hpp>
#include <engine/window/window.hpp>

namespace game
{

// Genric template we expect a library to provide an implementation of.
template<typename P>
struct game_policy_template
{
  game_policy_template() = delete;

  using window = ::engine::window::window;

  inline static decltype(auto)
  game_loop(window &&w)
  { return P::game_loop(std::forward<window>(w)); }

  // Export this for consumption through this policy.
  using game_type = typename P::game_type;
};

// Here we choose to select our the_library sdl::policy implementation, effectively selecting SDL
// as our windowing library.
//
// Isn't that exciting kids? ...
using the_library = game_policy_template<boomhs_policy>;
using game = the_library::game_type; // RE-EXPORT

} // ns game
