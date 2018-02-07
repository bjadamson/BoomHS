#pragma once
#include <window/sdl.hpp>

namespace window
{
struct FrameTime;
} // ns window

namespace boomhs
{

struct GameState;

struct IO {
  static void process(GameState &, SDL_Event &, window::FrameTime const&);
};

} // ns boomhs
