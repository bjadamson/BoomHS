#include <window/controller.hpp>
#include <stlw/format.hpp>

using namespace window;

namespace
{

auto
read_axis(SDL_GameControllerAxis const axis, Controller const& c)
{
  return SDL_GameControllerGetAxis(c.controller.get(), axis);
}

bool
is_pressed(SDL_GameControllerButton const button, Controller const& c)
{
  return (1 == SDL_JoystickGetButton(c.joystick, button));
}

} // ns anon

namespace window
{

void
destroy_controller(SDL_GameController *controller)
{
  assert(controller);
  SDL_GameControllerClose(controller);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Controller
std::ostream&
operator<<(std::ostream &stream, Controller const& c)
{
  // clang-format off

  // TODO: It seems like the SDL controller mapping for my xbox 360 is wrong? I need to dig into
  // it, but aside from that, there's enough here for full controller suport. :D
  auto const fmt = fmt::format(
      "left_x:       {} left_y: {}\n"
      "right_x:      {} right_y: {}\n"
      "trigger_left: {} trigger_right: {}\n"

      "a: {}, b: {}, x: {}, y: {}\n"
      "back: {}, guide: {}, start: {}\n"
      "js_left: {}, js_right: {}\n"
      "shoulder_left: {}, shoulder_right: {}\n"

      "dpad_down: {}, dpad_up: {}\n"
      "dpad_left: {}, dpad_right: {}\n",

      c.left_axis_x(),  c.left_axis_y(),
      c.right_axis_x(), c.right_axis_y(),

      c.left_bumper(),  c.right_bumper(),

      c.button_a(),     c.button_b(),    c.button_x(),     c.button_y(),
      c.button_back(),  c.button_back(), c.button_guide(), c.button_start(),

      c.button_left_joystick(), c.button_right_joystick(),
      c.button_left_shoulder(), c.button_right_shoulder(),
      c.button_dpad_down(), c.button_dpad_up(),
      c.button_dpad_left(), c.button_dpad_right());
  // clang-format on
  return stream;
}

bool
Controller::button_a() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_A, *this);
}

bool
Controller::button_b() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_B, *this);
}

bool
Controller::button_x() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_X, *this);
}

bool
Controller::button_y() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_Y, *this);
}

bool
Controller::button_back() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_BACK, *this);
}

bool
Controller::button_guide() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_GUIDE, *this);
}

bool
Controller::button_start() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_START, *this);
}

bool
Controller::button_left_joystick() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_LEFTSTICK, *this);
}

bool
Controller::button_right_joystick() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_RIGHTSTICK, *this);
}

bool
Controller::button_left_shoulder() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, *this);
}

bool
Controller::button_right_shoulder() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, *this);
}

bool
Controller::button_dpad_down() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN, *this);
}

bool
Controller::button_dpad_up() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_DPAD_UP, *this);
}

bool
Controller::button_dpad_left() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT, *this);
}

bool
Controller::button_dpad_right() const
{
  return is_pressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, *this);
}

int16_t
Controller::left_axis_x() const
{
  return read_axis(SDL_CONTROLLER_AXIS_LEFTX, *this);
}

int16_t
Controller::left_axis_y() const
{
  return read_axis(SDL_CONTROLLER_AXIS_LEFTY, *this);
}

int16_t
Controller::right_axis_x() const
{
  return read_axis(SDL_CONTROLLER_AXIS_RIGHTX, *this);
}

int16_t
Controller::right_axis_y() const
{
  return read_axis(SDL_CONTROLLER_AXIS_RIGHTY, *this);
}

int16_t
Controller::left_bumper() const
{
  return read_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, *this);
}

int16_t
Controller::right_bumper() const
{
  return read_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, *this);
}

void
SDLControllers::add(ControllerPTR &&ptr, SDL_Joystick *joystick)
{
  assert(joystick);

  Controller c{MOVE(ptr), joystick};
  controllers_.emplace_back(MOVE(c));
}

stlw::result<SDLControllers, std::string>
SDLControllers::find_attached_controllers(stlw::Logger &logger)
{
  int const num_controllers = SDL_NumJoysticks();
  LOG_INFO("Detected {} controllers plugged into the system");

  SDL_GameController *controller = nullptr;
  for (int i = 0; i < num_controllers; ++i) {
    if (!SDL_IsGameController(i)) {
      continue;
    }
    SDL_GameController *ci = SDL_GameControllerOpen(i);
    LOG_INFO(fmt::sprintf("Found controller: '%s'", SDL_GameControllerNameForIndex(i)));
    if (ci) {
      controller = ci;
      break;
    } else {
      return stlw::make_error(fmt::sprintf("Could not open gamecontroller %i: %s\n", i, SDL_GetError()));
    }
  }
  SDLControllers controllers;
  auto ptr = ControllerPTR{controller, &destroy_controller};
  SDL_Joystick *joystick = SDL_GameControllerGetJoystick(ptr.get());

  assert(joystick);
  controllers.add(MOVE(ptr), joystick);
  return controllers;
}

} // ns windo
