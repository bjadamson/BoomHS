#include <window/timer.hpp>
#include <extlibs/sdl.hpp>

namespace window
{

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
  auto const dt = clock_.frame_time().delta_ticks();
  clock_.update();
  if (!paused_) {
    // NOTE: since remaining_ is a unsigned value, we must check that remaining_ is atleast as big
    // as dt, otherwise the subtraction would yield an underflowed value.
    //
    // This was a bug that I do not wish to track down again.
    remaining_ = (dt > remaining_) ? 0 : (remaining_ - dt);
  }
}

} // namespace window
