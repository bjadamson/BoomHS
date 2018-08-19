#pragma once
#include <extlibs/sdl.hpp>

namespace boomhs
{
class Camera;
class FrameTime;
struct GameState;
class SDLControllers;

struct IO
{
  static void process(GameState&, SDLControllers const&, Camera&, FrameTime const&);

  static void process_event(GameState&, SDL_Event&, Camera&, FrameTime const&);
};

} // namespace boomhs
