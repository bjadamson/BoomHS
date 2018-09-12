#pragma once
#include <extlibs/sdl.hpp>

#include <type_traits>

namespace boomhs
{
class Camera;
struct GameState;
class FrameTime;
class Player;
class SDLControllers;
class WorldObject;

struct MouseAndKeyboardArgs
{
  GameState&       game_state;
  Camera&          camera;
  WorldObject&     player;
  FrameTime const& frame_time;
};

struct ControllerArgs
{
  GameState&            game_state;
  SDLControllers const& controllers;
  Player&               player;
  Camera&               camera;
  FrameTime const&      frame_time;
};

struct MouseButtonEvent
{
  GameState&            game_state;
  Camera&               camera;
  SDL_MouseButtonEvent& event;
  WorldObject&          player;
  FrameTime const&      frame_time;
};

struct MouseMotionEvent
{
  GameState&                  game_state;
  SDL_MouseMotionEvent const& motion;
  Camera&                     camera;
  Player&                     player;
  FrameTime const&            frame_time;
};

struct MouseWheelEvent
{
  GameState&                 game_state;
  SDL_MouseWheelEvent const& wheel;
  Camera&                    camera;
  WorldObject&               player;
  FrameTime const&           frame_time;
};

struct KeyEvent
{
  GameState&       game_state;
  SDL_Event const& event;
  Camera&          camera;
  Player&          player;
  FrameTime const& frame_time;
};

using OnMouseDown   = std::add_pointer<void(MouseButtonEvent&&)>::type;
using OnMouseUp     = OnMouseDown;
using OnMouseMotion = std::add_pointer<void(MouseMotionEvent&&)>::type;
using OnMouseWheel  = std::add_pointer<void(MouseWheelEvent&&)>::type;
using OnKeyDown     = std::add_pointer<void(KeyEvent&&)>::type;
using OnKeyUp       = OnKeyDown;

using ControllerUpdate    = std::add_pointer<void(ControllerArgs&&)>::type;
using MouseKeyboardUpdate = std::add_pointer<void(MouseAndKeyboardArgs&&)>::type;

struct PlayerBehavior
{
  OnMouseDown   on_mouse_down;
  OnMouseUp     on_mouse_up;
  OnMouseMotion on_mouse_motion;
  OnMouseWheel  on_mouse_wheel;

  OnKeyDown on_key_down;
  OnKeyUp   on_key_up;

  MouseKeyboardUpdate update_keyboard;
  MouseKeyboardUpdate update_mouse;
  ControllerUpdate    update_controller;
};

struct PlayerPlayingGameBehavior
{
  static void mousebutton_down(MouseButtonEvent&&);
  static void mousebutton_up(MouseButtonEvent&&);

  static void mouse_motion(MouseMotionEvent&&);
  static void mouse_wheel(MouseWheelEvent&&);

  static void keydown(KeyEvent&&);
  static void keyup(KeyEvent&&);

  static void process_mouse_state(MouseAndKeyboardArgs&&);
  static void process_keyboard_state(MouseAndKeyboardArgs&&);
  static void process_controller_state(ControllerArgs&&);
};

struct TerminalOnlyBehavior
{
  static void mousebutton_down(MouseButtonEvent&&);
  static void mousebutton_up(MouseButtonEvent&&);

  static void mouse_motion(MouseMotionEvent&&);
  static void mouse_wheel(MouseWheelEvent&&);

  static void keydown(KeyEvent&&);
  static void keyup(KeyEvent&&);

  static void process_mouse_state(MouseAndKeyboardArgs&&);
  static void process_keyboard_state(MouseAndKeyboardArgs&&);
  static void process_controller_state(ControllerArgs&&);
};

struct PlayerBehaviors
{
  PlayerBehavior const* active = nullptr;

  PlayerBehavior const player_playing_behavior{
      &PlayerPlayingGameBehavior::mousebutton_down,
      &PlayerPlayingGameBehavior::mousebutton_up,

      &PlayerPlayingGameBehavior::mouse_motion,
      &PlayerPlayingGameBehavior::mouse_wheel,

      &PlayerPlayingGameBehavior::keydown,
      &PlayerPlayingGameBehavior::keyup,

      &PlayerPlayingGameBehavior::process_mouse_state,
      &PlayerPlayingGameBehavior::process_keyboard_state,
      &PlayerPlayingGameBehavior::process_controller_state};
  PlayerBehavior const terminal_behavior{&TerminalOnlyBehavior::mousebutton_down,
                                         &TerminalOnlyBehavior::mousebutton_up,

                                         &TerminalOnlyBehavior::mouse_motion,
                                         &TerminalOnlyBehavior::mouse_wheel,

                                         &TerminalOnlyBehavior::keydown,
                                         &TerminalOnlyBehavior::keyup,

                                         &TerminalOnlyBehavior::process_mouse_state,
                                         &TerminalOnlyBehavior::process_keyboard_state,
                                         &TerminalOnlyBehavior::process_controller_state};
};

} // namespace boomhs
