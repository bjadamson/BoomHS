#pragma once
#include <extlibs/sdl.hpp>

namespace window
{
class SDLControllers;
} // namespace window

namespace boomhs
{
class Camera;
class FrameTime;
struct GameState;

struct IO
{
  static void process(GameState&, window::SDLControllers const&, Camera&, FrameTime const&);

  static void process_event(GameState&, SDL_Event&, Camera&, FrameTime const&);
};

} // namespace boomhs
