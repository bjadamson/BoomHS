#include <boomhs/gcd.hpp>

namespace boomhs
{

void
GCD::reset_ms(common::ticks_t const t)
{
  stopwatch_.set_ms(t);
}

} // namespace boomhs
