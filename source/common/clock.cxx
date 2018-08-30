#include <common/clock.hpp>
#include <extlibs/sdl.hpp>

namespace common
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

ticks_t
Clock::delta_ticks_since_last_update() const
{
  return now() - last_;
}

ticks_t
Clock::delta_millis_since_last_update() const
{
  ticks_t const dt = delta_ticks_since_last_update();
  return common::TimeConversions::ticks_to_millis(dt, frequency());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// StopWatch
void
StopWatch::update()
{
  auto const dt = clock_.delta_millis_since_last_update();

  clock_.update();
  if (!paused_) {
    remaining_ms_ -= dt;
  }
}

} // namespace common
