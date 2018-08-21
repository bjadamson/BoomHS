#include <boomhs/clock.hpp>
#include <extlibs/sdl.hpp>

#include <iostream>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
ticks_t
TimeConversions::ticks_to_seconds(ticks_t const ticks, freq_t const freq)
{
  auto const millis = ticks_to_millis(ticks, freq);
  return millis_to_seconds(ticks);
}

ticks_t
TimeConversions::seconds_to_ticks(ticks_t const ticks, freq_t const freq)
{
  auto const millis = seconds_to_millis(ticks);
  return millis_to_ticks(ticks, freq);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Clock
Clock::Clock()
    : frequency_(SDL_GetPerformanceFrequency())
    , start_(now())
    , last_(start_)
{
}

ticks_t
Clock::now() const
{
  return SDL_GetPerformanceCounter();
}

FrameTime
Clock::frame_time() const
{
  ticks_t const delta = now() - last_;
  return FrameTime{delta, since_start(), frequency_};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Timer
void
Timer::update()
{
  auto const dt = clock_.frame_time().delta_millis();
  clock_.update();
  if (!paused_) {
    remaining_ms_ -= dt;
  }
}

} // namespace boomhs
