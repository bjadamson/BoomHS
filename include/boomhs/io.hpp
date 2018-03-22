#pragma once
#include <extlibs/sdl.hpp>

namespace window
{
class FrameTime;
class SDLControllers;
} // namespace window

namespace boomhs
{

struct GameState;

struct IO
{
  static void
  process(GameState&, SDL_Event&, window::SDLControllers const&, window::FrameTime const&);
};

} // namespace boomhs
