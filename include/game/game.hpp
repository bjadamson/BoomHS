#pragma once
#include <engine/window/window.hpp>
#include <stlw/type_macros.hpp>

namespace game
{

class game_data
{
  using window = ::engine::window::window;

  NO_COPY(game_data);

  // private ctor
  game_data(window &&w, float const& t) : window_(std::move(w)), triangle_(t) {}

public:
  // fields
  window window_;
  float const& triangle_;

  MOVE_DEFAULT(game_data);

  template<typename ...P>
  game_data
  static make(P &&...p) { return game_data{std::forward<P>(p)...}; }
};

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

template<typename P>
using the_library = game_policy_template<P>;

template<typename P>
using game = typename the_library<P>::game_type; // RE-EXPORT

} // ns game
