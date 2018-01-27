#pragma once
#include <window/sdl.hpp>

namespace boomhs
{

struct GameState;

struct IO {
  static void process(GameState &, SDL_Event &, double const);
};

} // ns boomhs
