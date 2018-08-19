#include <boomhs/gcd.hpp>

namespace boomhs
{

void
GCD::reset_ms(ticks_t const t)
{
  timer_.set_ms(t);
}

} // namespace boomhs
