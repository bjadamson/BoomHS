#pragma once

namespace engine::gfx
{
enum class draw_mode
{
  TRIANGLES,
  TRIANGLE_STRIP,
  TRIANGLE_FAN,
  LINE_LOOP,

  INVALID_DRAW_MODE
};

} // ns engine::gfx
