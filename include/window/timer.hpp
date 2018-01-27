#pragma once
#include <window/sdl.hpp>
#include <stlw/log.hpp>

namespace window
{

struct Clock
{
  // The clock time when the timer started
  uint32_t starting_ticks_ = SDL_GetTicks();
  uint32_t last_tick_time = 0;
  uint32_t delta = 0;

  Clock() = default;

  uint32_t
  ticks_now() const
  {
    return SDL_GetTicks();
  }

  void
  update(stlw::Logger &logger)
  {
    uint32_t const tick_time = ticks_now();
    delta = tick_time - last_tick_time;
    last_tick_time = tick_time;
  }
};

struct FrameCounter
{
  int frames_counted = 0;

  void
  update(stlw::Logger &logger, Clock const& clock)
  {
    float const fps = frames_counted / (clock.ticks_now() / 1000.0f);
    LOG_INFO(fmt::format("average FPS '{}'", fps));
    ++frames_counted;
  }
};

} // ns window
