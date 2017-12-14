#pragma once
#include <boomhs/state.hpp>
#include <window/sdl.hpp>

namespace boomhs
{

struct IO {
  static void process(GameState &, SDL_Event &);
};

} // ns boomhs
