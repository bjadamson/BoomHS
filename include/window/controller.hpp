#pragma once
#include <window/sdl.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <memory>

namespace window
{

void
destroy_controller(SDL_GameController *);
using ControllerPTR = std::unique_ptr<SDL_GameController, decltype(&destroy_controller)>;

class SDLControllers
{
  std::vector<ControllerPTR> controllers_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(SDLControllers);
  SDLControllers() = default;

  void
  add(ControllerPTR &&ptr);

  static stlw::result<SDLControllers, std::string>
  find_attached_controllers(stlw::Logger &);
};


} // ns windo
