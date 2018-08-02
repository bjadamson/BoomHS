#pragma once
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace window
{

using ticks_t = uint64_t;

class FrameTime
{
  ticks_t const delta_, since_start_;
  double const  frequency_;

  double ticks_to_millis(ticks_t const t) const { return t * 1000.0 / frequency_; }
  double millis_to_seconds(double const m) const { return m * 0.001; }

public:
  COPY_DEFAULT(FrameTime);
  MOVE_DEFAULT(FrameTime);
  explicit FrameTime(ticks_t const dt, ticks_t const sstart, double const fr)
      : delta_(dt)
      , since_start_(sstart)
      , frequency_(fr)
  {
  }

  double delta_ticks() const { return delta_; }
  double delta_millis() const { return ticks_to_millis(delta_ticks()); }
  double delta_seconds() const { return millis_to_seconds(delta_millis()); }

  double since_start_millis() const { return ticks_to_millis(since_start_); }
  double since_start_seconds() const { return millis_to_seconds(since_start_millis()); }
};

// The clock time when the timer started
class Clock
{
  double  frequency_;
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
  bool    expired_   = false;
  bool    paused_    = false;
  ticks_t remaining_ = 0.0;

public:
  COPY_DEFAULT(Timer);
  MOVE_DEFAULT(Timer);
  Timer() = default;

  void set(ticks_t const t) { remaining_ = t; }
  bool expired() const { return remaining_ <= 0; }
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
