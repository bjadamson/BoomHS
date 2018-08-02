#include <window/timer.hpp>
#include <extlibs/sdl.hpp>

#include <iostream>
namespace window
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TimeConversions
ticks_t
TimeConversions::ticks_to_millis(ticks_t const t, freq_t const freq)
{
  assert(t != 0.0);
  double const freq_d = static_cast<double>(SDL_GetPerformanceFrequency());
  return t * 1000.0 / freq_d;
}

ticks_t
TimeConversions::millis_to_ticks(ticks_t const t, freq_t const freq)
{
  double const freq_d = static_cast<double>(SDL_GetPerformanceFrequency());
  return t / 1000.0 * freq_d;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ticks_t
TimeConversions::millis_to_seconds(ticks_t const m)
{
  return m * 0.001;
}

ticks_t
TimeConversions::seconds_to_millis(ticks_t const s)
{
  return s * 1000.0;
}


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

} // namespace window
