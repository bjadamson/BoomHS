#pragma once
#include <window/sdl.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace window
{

using ticks_t = uint64_t;

class FrameTime
{
  ticks_t const delta_, since_start_;
  double const frequency_;

  auto delta() const { return delta_; }
  auto ticks_to_millis(ticks_t const t) const { return t * 1000.0 / frequency_; }
  auto millis_to_seconds(double const m) const { return m * 0.001; }

public:
  explicit FrameTime(ticks_t const dt, ticks_t const sstart, double const fr)
    : delta_(dt)
    , since_start_(sstart)
    , frequency_(fr)
  {
  }

  double delta_millis() const { return ticks_to_millis(delta()); }
  double delta_seconds() const { return millis_to_seconds(delta_millis()); }

  auto since_start_millis() const { return ticks_to_millis(since_start_); }
  auto since_start_seconds() const { return millis_to_seconds(since_start_millis()); }
};

// The clock time when the timer started
class Clock
{
  double const frequency_;
  ticks_t const start_;
  ticks_t last_;

  ticks_t now() const { return SDL_GetPerformanceCounter(); }
  ticks_t since_start() const { return now() - start_; }
public:
  NO_COPY_AND_NO_MOVE(Clock);
  Clock()
    : frequency_(SDL_GetPerformanceFrequency())
    , start_(now())
    , last_(start_)
  {
  }

  void
  update()
  {
    last_ = now();
  }

  FrameTime frame_time() const
  {
    ticks_t const delta = now() - last_;
    return FrameTime{delta, since_start(), frequency_};
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
