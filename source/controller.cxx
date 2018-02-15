#include <window/controller.hpp>
#include <stlw/format.hpp>

namespace window
{

void
destroy_controller(SDL_GameController *controller)
{
  assert(controller);
  SDL_GameControllerClose(controller);
}

void
SDLControllers::add(ControllerPTR &&ptr)
{
  controllers_.emplace_back(MOVE(ptr));
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
    if (ci) {
      controller = ci;
      break;
    } else {
      return stlw::make_error(fmt::sprintf("Could not open gamecontroller %i: %s\n", i, SDL_GetError()));
    }
  }
  SDLControllers controllers;
  auto ptr = ControllerPTR{controller, &destroy_controller};
  controllers.add(MOVE(ptr));
  return controllers;
}

} // ns windo
