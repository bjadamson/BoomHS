#pragma once
#include <common/clock.hpp>
#include <common/time.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{

using ticks_t = ::common::ticks_t;
using freq_t  = ::common::freq_t;

class FrameTime
{
  ticks_t const delta_, since_start_;
  freq_t const  frequency_;

public:
  NO_MOVE_OR_COPY(FrameTime);
  explicit FrameTime(ticks_t const dt, ticks_t const sstart, freq_t const fr)
      : delta_(dt)
      , since_start_(sstart)
      , frequency_(fr)
  {
  }

  ticks_t delta_ticks() const { return delta_; }
  ticks_t delta_millis() const
  {
    return common::TimeConversions::ticks_to_millis(delta_ticks(), frequency_);
  }
  ticks_t delta_seconds() const { return common::TimeConversions::millis_to_seconds(delta_millis()); }

  ticks_t since_start_millis() const
  {
    return common::TimeConversions::ticks_to_millis(since_start_, frequency_);
  }
  ticks_t since_start_seconds() const
  {
    return common::TimeConversions::millis_to_seconds(since_start_millis());
  }

  static FrameTime from_clock(common::Clock const& clock)
  {
    ticks_t const delta = clock.delta_ticks_since_last_update();
    return FrameTime{delta, clock.since_start(), clock.frequency()};
  }
};

struct FrameCounter
{
  int64_t frames_counted = 0u;

  void update(common::Logger& logger, common::Clock const& clock) { ++frames_counted; }
};

} // namespace boomhs
