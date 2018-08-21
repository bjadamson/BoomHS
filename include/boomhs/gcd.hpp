#pragma once
#include <boomhs/clock.hpp>

namespace boomhs
{
struct Item;

class GCD
{
  Timer timer_;

public:
  GCD() = default;
  MOVE_DEFAULT(GCD);
  COPY_DEFAULT(GCD);

  bool is_ready() const { return timer_.expired(); }

  bool is_paused() const { return timer_.is_paused(); }
  void pause() { timer_.pause(); }
  void unpause() { timer_.unpause(); }

  void reset_ms(ticks_t);
  void update() { timer_.update(); }
};

} // namespace boomhs
