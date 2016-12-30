#pragma once

namespace gfx
{
enum class draw_mode
{
  TRIANGLES = 0u,
  TRIANGLE_STRIP,
  TRIANGLE_FAN,
  LINE_LOOP,

  INVALID_DRAW_MODE
};

} // ns gfx