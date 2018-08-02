#pragma once
#include <window/timer.hpp>

namespace boomhs
{
struct Item;

class GCD
{
  window::Timer timer_;

public:
  GCD() = default;
  MOVE_DEFAULT(GCD);
  COPY_DEFAULT(GCD);

  bool is_ready() const { return timer_.expired(); }

  bool is_paused() const { return timer_.is_paused(); }
  void pause() { timer_.pause(); }
  void unpause() { timer_.unpause(); }

  void reset(window::ticks_t const t) { timer_.set(t); }
  void update() { timer_.update(); }
};

} // namespace boomhs
