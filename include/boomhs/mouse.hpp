#pragma once
#include <boomhs/device.hpp>

#include <common/type_macros.hpp>
#include <extlibs/sdl.hpp>

namespace boomhs
{

struct ScreenCoordinates
{
  int x, y;
};

class MouseState
{
  auto mask() const { return SDL_GetMouseState(nullptr, nullptr); }

public:
  MouseState() = default;

  auto coords() const
  {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return ScreenCoordinates{x, y};
  }
  bool left_pressed() const { return mask() & SDL_BUTTON(SDL_BUTTON_LEFT); }
  bool right_pressed() const { return mask() & SDL_BUTTON(SDL_BUTTON_RIGHT); }
  bool middle_pressed() const { return mask() & SDL_BUTTON(SDL_BUTTON_MIDDLE); }

  bool both_pressed() const { return left_pressed() && right_pressed(); }
  bool either_pressed() const { return left_pressed() || right_pressed(); }
};

struct MouseStates
{
  MouseState current;
  MouseState previous;
};

} // namespace boomhs
