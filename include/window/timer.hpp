#pragma once
#include <window/sdl.hpp>
#include <stlw/log.hpp>

namespace window
{

struct Clock
{
  // The clock time when the timer started
  uint64_t const starting_ticks_ = SDL_GetTicks();
  uint64_t last_tick_time = starting_ticks_;

  Clock() = default;

  uint64_t
  ticks_now() const
  {
    return SDL_GetPerformanceCounter();
  }

  void
  update(stlw::Logger &logger)
  {
    last_tick_time = ticks_now();
  }

  auto ticks() const { return ticks_now() - last_tick_time; }
};

struct FrameCounter
{
  int64_t frames_counted = 0u;

  void
  update(stlw::Logger &logger, Clock const& clock)
  {
    //int64_t const ticks = clock.ticks_now();
    //double const fps = frames_counted / (ticks / 1000.0);
    //LOG_INFO(fmt::format("FPS '{}'", fps));
    ++frames_counted;
  }
};

} // ns window
