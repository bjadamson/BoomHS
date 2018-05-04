#pragma once
#include <extlibs/sdl.hpp>

namespace window
{
class FrameTime;
class SDLControllers;
} // namespace window

namespace boomhs
{
class Camera;
struct GameState;

struct IO
{
  static void process(GameState&, window::SDLControllers const&, Camera&, window::FrameTime const&);

  static void process_event(GameState&, SDL_Event&, Camera&, window::FrameTime const&);
};

} // namespace boomhs
