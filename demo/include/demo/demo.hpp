#pragma once
#include <boomhs/mouse.hpp>

namespace demo
{

enum ScreenSector
{
  LEFT_TOP = 0,
  RIGHT_TOP,
  LEFT_BOTTOM,
  RIGHT_BOTTOM,
  MAX
};

struct MouseCursorInfo
{
  ScreenSector sector;
  boomhs::MouseClickPositions click_positions;
};

} // namespace demo
