#pragma once
#include <window/sdl.hpp>
#include <stlw/log.hpp>

namespace window
{

struct FrameTime
{
  double const delta;
  uint64_t const ticks;
};

class Clock
{
  // The clock time when the timer started
  uint64_t const frequency_ = SDL_GetPerformanceFrequency();

  uint64_t ticks_now() const { return SDL_GetPerformanceCounter(); }
public:
  uint64_t const starting_ticks = ticks_now();
  uint64_t last_tick_time = starting_ticks;

  Clock() = default;

  void
  update(stlw::Logger &logger)
  {
    last_tick_time = ticks_now();
  }

  FrameTime frame_time() const
  {
    uint64_t const ticks = ticks_now() - last_tick_time;
    double const dt = (ticks * 1000.0 / frequency_);
    return FrameTime{dt, ticks};
  }
};

struct FrameCounter
{
  int64_t frames_counted = 0u;

  void
  update(stlw::Logger &logger, Clock const& clock)
  {
    ++frames_counted;
  }
};

} // ns window
