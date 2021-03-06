#pragma once
#include <boomhs/device.hpp>

#include <array>
#include <common/type_macros.hpp>
#include <extlibs/glm.hpp>
#include <extlibs/sdl.hpp>

namespace boomhs
{

class CursorManager
{
  static auto constexpr CURSOR_INDEX_BEGIN = SDL_SYSTEM_CURSOR_ARROW;
  static auto constexpr CURSOR_INDEX_END   = SDL_NUM_SYSTEM_CURSORS;

  static_assert(CURSOR_INDEX_END > CURSOR_INDEX_BEGIN, "CursorEndIndex Must Be > CursorBeginIndex");
  static auto constexpr CURSOR_COUNT = CURSOR_INDEX_END - CURSOR_INDEX_BEGIN - 1;

  std::array<SDL_Cursor*, CURSOR_INDEX_END - CURSOR_INDEX_BEGIN> cursors;
  SDL_SystemCursor                                               active_ = CURSOR_INDEX_BEGIN;

  bool contains(SDL_SystemCursor) const;

public:
  NO_COPY(CursorManager);
  MOVE_DEFAULT(CursorManager);

  void        set_active(SDL_SystemCursor);
  SDL_Cursor* active() const;

  CursorManager();
  ~CursorManager();
};

struct MouseClickPositions {
  glm::ivec2 left_right;
  glm::ivec2 middle;
};

class MouseState
{
  auto mask() const { return SDL_GetMouseState(nullptr, nullptr); }

public:
  MouseState() = default;

  glm::ivec2 coords() const;
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
