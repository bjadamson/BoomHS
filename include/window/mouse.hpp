#pragma once
#include <window/sdl.hpp>

namespace window
{

struct mouse_state
{
  int x, y;
  uint32_t mask;
};

auto mouse_position_now()
{
  int x, y;
  auto const mask = SDL_GetMouseState(&x, &y);
  return mouse_state{x, y, mask};
}

class mouse_data
{
  mouse_state prev_;
  mouse_state current_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(mouse_data);
  explicit constexpr mouse_data(mouse_state const& prev, mouse_state const& curr)
    : prev_(prev)
    , current_(curr)
  {
  }

  auto const& prev() const { return this->prev_; }
  auto const& current() const { return this->current_; }

  void add(mouse_state const& ms)
  {
    this->prev_ = this->current_;
    this->current_ = ms;
  }
};

auto make_default_mouse_data()
{
  auto const md = mouse_position_now();
  return mouse_data{md, md};
}

} // ns window
