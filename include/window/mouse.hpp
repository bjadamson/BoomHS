#pragma once
#include <window/sdl.hpp>

namespace window
{

struct mouse_state
{
  int x, y, xrel, yrel;
  uint32_t mask;
};

class mouse_data
{
  mouse_state prev_;
  mouse_state current_;

  void add_impl(mouse_state const& ms)
  {
    this->prev_ = this->current_;
    this->current_ = ms;
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(mouse_data);
  explicit constexpr mouse_data(mouse_state const& prev, mouse_state const& curr)
    : prev_(prev)
    , current_(curr)
  {
  }

  auto const& prev() const { return this->prev_; }
  auto const& current() const { return this->current_; }

  template<typename E>
  void add_from_event(E const& event)
  {
    auto const& m = event.motion;
    mouse_state const ms{m.x, m.y, m.xrel, m.yrel, m.state};
    add_impl(ms);
  }
};

namespace impl
{

auto mouse_position_now()
{
  int x, y;
  int constexpr xrel = 0, yrel = 0;
  auto const mask = SDL_GetMouseState(&x, &y);
  return mouse_state{x, y, xrel, yrel, mask};
}

} // ns impl

auto make_default_mouse_data()
{
  auto const md = impl::mouse_position_now();
  return mouse_data{md, md};
}

} // ns window
