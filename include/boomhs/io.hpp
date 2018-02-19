#pragma once
#include <window/sdl.hpp>

namespace window
{
class FrameTime;
class SDLControllers;
} // ns window

namespace boomhs
{

struct GameState;

struct IO {
  static void process(GameState &, SDL_Event &, window::SDLControllers const&, window::FrameTime const&);
};

} // ns boomhs
