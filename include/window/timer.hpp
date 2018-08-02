#pragma once
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace window
{

using ticks_t = double;
using freq_t  = int64_t;

struct TimeConversions
{
  TimeConversions() = delete;

  static ticks_t ticks_to_millis(ticks_t, freq_t);
  static ticks_t millis_to_ticks(ticks_t, freq_t);

  static ticks_t millis_to_seconds(ticks_t);
  static ticks_t seconds_to_millis(ticks_t);

  static ticks_t ticks_to_seconds(ticks_t, freq_t);
  static ticks_t seconds_to_ticks(ticks_t, freq_t);
};

class FrameTime
{
  ticks_t const delta_, since_start_;
  freq_t const  frequency_;

public:
  COPY_DEFAULT(FrameTime);
  MOVE_DEFAULT(FrameTime);
  explicit FrameTime(ticks_t const dt, ticks_t const sstart, freq_t const fr)
      : delta_(dt)
      , since_start_(sstart)
      , frequency_(fr)
  {
  }

  ticks_t delta_ticks() const { return delta_; }
  ticks_t delta_millis() const
  {
    return TimeConversions::ticks_to_millis(delta_ticks(), frequency_);
  }
  ticks_t delta_seconds() const { return TimeConversions::millis_to_seconds(delta_millis()); }

  ticks_t since_start_millis() const
  {
    return TimeConversions::ticks_to_millis(since_start_, frequency_);
  }
  ticks_t since_start_seconds() const
  {
    return TimeConversions::millis_to_seconds(since_start_millis());
  }
};

// The clock time when the timer started
class Clock
{
  freq_t  frequency_;
  ticks_t start_;
  ticks_t last_;

  ticks_t now() const;
  ticks_t since_start() const { return now() - start_; }

public:
  COPY_DEFAULT(Clock);
  MOVE_DEFAULT(Clock);

  Clock();
  void update() { last_ = now(); }

  FrameTime frame_time() const;
};

class Timer
{
  Clock   clock_;
  bool    expired_      = false;
  bool    paused_       = false;
  ticks_t remaining_ms_ = 0.0;

public:
  COPY_DEFAULT(Timer);
  MOVE_DEFAULT(Timer);
  Timer() = default;

  void set_ms(ticks_t const t) { remaining_ms_ = t; }
  bool expired() const { return remaining_ms_ <= 0; }
  bool is_paused() const { return paused_; }
  void pause() { paused_ = true; }
  void unpause() { paused_ = false; }

  void update();
};

struct FrameCounter
{
  int64_t frames_counted = 0u;

  void update(stlw::Logger& logger, Clock const& clock) { ++frames_counted; }
};

} // namespace window
