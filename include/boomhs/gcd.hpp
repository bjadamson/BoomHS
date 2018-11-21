#pragma once
#include <common/timer.hpp>

namespace boomhs
{
struct Item;

class GCD
{
  common::StopWatch stopwatch_;

public:
  GCD() = default;
  MOVE_DEFAULT(GCD);
  COPY_DEFAULT(GCD);

  bool is_ready() const { return stopwatch_.expired(); }

  bool is_paused() const { return stopwatch_.is_paused(); }
  void pause() { stopwatch_.pause(); }
  void unpause() { stopwatch_.unpause(); }

  void reset_ms(common::ticks_t);
  void update() { stopwatch_.update(); }
};

} // namespace boomhs
