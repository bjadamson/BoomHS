#pragma once
#include <window/sdl.hpp>

namespace window
{

class LTimer
{
  //The clock time when the timer started
  Uint32 mStartTicks = SDL_GetTicks();
public:
  LTimer() = default;

  uint32_t get_ticks() const { return SDL_GetTicks() - mStartTicks; }
};

} // ns window
