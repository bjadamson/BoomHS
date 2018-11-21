#pragma once

namespace boomhs
{
class Camera;
class FrameTime;
class GameState;
class SDLControllers;

struct SDLReadDevicesArgs
{
  GameState&            game_state;
  SDLControllers const& controllers;
  Camera&               camera;
  FrameTime const&      frame_time;
};

struct SDLEventProcessArgs
{
  GameState&       game_state;
  SDL_Event&       event;
  Camera&          camera;
  FrameTime const& frame_time;
};

struct IO_SDL
{
  static void read_devices(SDLReadDevicesArgs&&);

  static void process_event(SDLEventProcessArgs&&);
};

} // namespace boomhs
