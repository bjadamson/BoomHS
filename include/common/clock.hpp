#pragma once
#include <common/log.hpp>
#include <common/time.hpp>
#include <common/type_macros.hpp>

namespace common
{

// The clock time when the timer started
class Clock
{
  freq_t  frequency_;
  ticks_t start_;
  ticks_t last_;

  ticks_t now() const;

public:
  COPYMOVE_DEFAULT(Clock);
  Clock();

  void update() { last_ = now(); }
  ticks_t since_start() const { return now() - start_; }
  ticks_t delta_ticks_since_last_update() const;
  ticks_t delta_millis_since_last_update() const;

  auto frequency() const { return frequency_; }
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

} // namespace common
