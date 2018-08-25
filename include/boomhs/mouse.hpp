#pragma once
#include <boomhs/device.hpp>

#include <common/type_macros.hpp>
#include <extlibs/sdl.hpp>
#include <array>

namespace boomhs
{

struct ScreenCoordinates
{
  float x, y;
};

class CursorManager
{
  static auto constexpr CURSOR_INDEX_BEGIN = SDL_SYSTEM_CURSOR_ARROW;
  static auto constexpr CURSOR_INDEX_END   = SDL_NUM_SYSTEM_CURSORS;
  std::array<SDL_Cursor*, CURSOR_INDEX_END - CURSOR_INDEX_BEGIN> cursors;
  SDL_SystemCursor active_ = CURSOR_INDEX_BEGIN;

  bool contains(SDL_SystemCursor) const;
public:
  NO_COPY(CursorManager);
  MOVE_DEFAULT(CursorManager);

  void set_active(SDL_SystemCursor);
  SDL_Cursor* active() const;

  CursorManager();
  ~CursorManager();
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
    return ScreenCoordinates{static_cast<float>(x), static_cast<float>(y)};
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
