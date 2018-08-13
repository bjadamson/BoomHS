#pragma once
#include <common/log.hpp>
#include <common/result.hpp>
#include <extlibs/sdl.hpp>
#include <memory>
#include <ostream>

namespace window
{

void
destroy_controller(SDL_GameController*);
using ControllerPTR = std::unique_ptr<SDL_GameController, decltype(&destroy_controller)>;

struct Controller
{
  ControllerPTR controller;
  SDL_Joystick* joystick;

  MOVE_CONSTRUCTIBLE_ONLY(Controller);
  Controller() = delete;

  //
  // Axis
  int16_t axis_left_x() const;
  int16_t axis_left_y() const;

  int16_t axis_right_x() const;
  int16_t axis_right_y() const;

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

  bool button_left_trigger() const;
  bool button_right_trigger() const;

  bool button_dpad_down() const;
  bool button_dpad_up() const;
  bool button_dpad_left() const;
  bool button_dpad_right() const;
};

std::ostream&
operator<<(std::ostream&, Controller const&);

class SDLControllers
{
  std::vector<Controller> controllers_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(SDLControllers);
  SDLControllers() = default;

  void add(ControllerPTR&&, SDL_Joystick*);

  static Result<SDLControllers, std::string> find_attached_controllers(common::Logger&);

  auto const& first() const
  {
    assert(controllers_.size() > 0);
    return controllers_.front();
  }

  bool empty() const { return controllers_.empty(); }

  auto size() const { return controllers_.size(); }
};

} // namespace window
