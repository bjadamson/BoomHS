#pragma once
#include <window/sdl.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <ostream>
#include <memory>

namespace window
{

void
destroy_controller(SDL_GameController *);
using ControllerPTR = std::unique_ptr<SDL_GameController, decltype(&destroy_controller)>;

struct Controller
{
  ControllerPTR controller;
  SDL_Joystick *joystick;

  MOVE_CONSTRUCTIBLE_ONLY(Controller);
  Controller() = delete;

  //
  // Axis
  int16_t left_axis_x() const;
  int16_t left_axis_y() const;

  int16_t right_axis_x() const;
  int16_t right_axis_y() const;

  int16_t left_bumper() const;
  int16_t right_bumper() const;

  //
  // Buttons
  bool button_a() const;
  bool button_b() const;
  bool button_x() const;
  bool button_y() const;

  bool button_back() const;
  bool button_guide() const;
  bool button_start() const;

  bool button_left_joystick() const;
  bool button_right_joystick() const;

  bool button_left_shoulder() const;
  bool button_right_shoulder() const;

  bool button_dpad_down() const;
  bool button_dpad_up() const;
  bool button_dpad_left() const;
  bool button_dpad_right() const;
};

std::ostream&
operator<<(std::ostream &, Controller const&);

class SDLControllers
{
  std::vector<Controller> controllers_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(SDLControllers);
  SDLControllers() = default;

  void
  add(ControllerPTR &&, SDL_Joystick *);

  static stlw::result<SDLControllers, std::string>
  find_attached_controllers(stlw::Logger &);

  auto const&
  first() const
  {
    assert(controllers_.size() > 0);
    return controllers_.front();
  }

  auto size() const { return controllers_.size(); }
};

} // ns windo
