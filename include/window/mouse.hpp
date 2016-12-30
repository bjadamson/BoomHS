#pragma once
#include <window/sdl.hpp>

namespace window
{

struct mouse_state
{
  int x, y, xrel, yrel;
  uint32_t mask;
};

struct mouse_sensitivity
{
  float x, y;
};

struct mouse_data
{
  mouse_state prev;
  mouse_state current;
  mouse_sensitivity sensitivity;
  bool pitch_lock = true;

  MOVE_CONSTRUCTIBLE_ONLY(mouse_data);
};

template<typename E>
void add_from_event(mouse_data &md, E const& event)
{
  auto const& m = event.motion;
  mouse_state const ms{m.x, m.y, m.xrel, m.yrel, m.state};
  md.prev = md.current;
  md.current = ms;
}

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
  auto const init_md = impl::mouse_position_now();
  mouse_sensitivity const init_sensitivity{0.002f, 0.002f};

  return mouse_data{init_md, init_md, init_sensitivity};
}

} // ns window
